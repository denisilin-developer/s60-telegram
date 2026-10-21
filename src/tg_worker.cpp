#include <string.h>
#include <stdlib.h>

#include <f32file.h>
#include <utf.h>

#include "tg_worker.h"
#include "tg_mtproto.h"
#include "tg_config.h"


CTgWorker *CTgWorker::NewL(MTgObserver &aObserver, const char *aDir)
{
    CTgWorker *self = new (ELeave) CTgWorker(aObserver);
    CleanupStack::PushL(self);
    self->ConstructL(aDir);
    CleanupStack::Pop(self);
    return self;
}

CTgWorker::CTgWorker(MTgObserver &aObserver)
    : CActive(EPriorityStandard), iObserver(aObserver)
{
    CActiveScheduler::Add(this);
}

void CTgWorker::ConstructL(const char *aDir)
{
    strncpy(iDir, aDir, sizeof iDir - 1);
    iUiThread = RThread().Id();
    User::LeaveIfError(iWake.CreateLocal(0));
    User::LeaveIfError(iThread.Create(KNullDesC, ThreadMain, 0x14000, &User::Allocator(), this));
    iThread.Resume();
}

CTgWorker::~CTgWorker()
{
    Cancel();
    if (iThread.Handle()) {
        TRequestStatus died, timer;
        RTimer t;
        iQuit = ETrue;
        iThread.Logon(died);
        iWake.Signal();
        if (t.CreateLocal() == KErrNone) {
            t.After(timer, 3000000);
            User::WaitForRequest(died, timer);
            if (died == KRequestPending) {
                iThread.LogonCancel(died);
                User::WaitForRequest(died);
            } else {
                t.Cancel();
                User::WaitForRequest(timer);
            }
            t.Close();
        } else {
            iThread.LogonCancel(died);
            User::WaitForRequest(died);
        }
        iThread.Close();
    }
    iWake.Close();
    ClearOutputs();
}

TInt CTgWorker::Start()
{
    if (Busy())
        return KErrInUse;
    if (iInDone) {
        iDeferred = ETrue;
        return KErrNone;
    }
    Launch();
    return KErrNone;
}

void CTgWorker::Launch()
{
    iJob = iNext;
    iStatus = KRequestPending;
    iUiStatus = &iStatus;
    SetActive();
    iWake.Signal();
}

void CTgWorker::ClearOutputs()
{
    tg_dialogs_free(iJob.iDialogs, iJob.iNumDialogs);
    tg_folders_free(iJob.iFolders, iJob.iNumFolders);
    tg_dialogs_free(iJob.iArchive, iJob.iNumArchive);
    iJob.iArchive = NULL;
    iJob.iNumArchive = 0;
    iJob.iFolders = NULL;
    iJob.iNumFolders = 0;
    tg_messages_free(iJob.iMessages, iJob.iNumMessages);
    iJob.iDialogs = NULL;
    iJob.iNumDialogs = 0;
    iJob.iMessages = NULL;
    iJob.iNumMessages = 0;
    iJob.iResult = 0;
    iJob.iChanged = 0;
    free(iJob.iPinned);
    iJob.iPinned = NULL;
    tg_info_free(&iJob.iInfo);
    tg_dialogs_free(iJob.iContacts, iJob.iNumContacts);
    iJob.iContacts = NULL;
    iJob.iNumContacts = 0;
    free(iJob.iResolved.name);
    free(iJob.iResolved.text);
    memset(&iJob.iResolved, 0, sizeof iJob.iResolved);
    iJob.iError[0] = 0;
    iJob.iCodeType[0] = 0;
}

void CTgWorker::RunL()
{
    iInDone = ETrue;
    TRAPD(err, iObserver.JobDoneL(iJob));
    iInDone = EFalse;
    ClearOutputs();
    if (iDeferred) {
        iDeferred = EFalse;
        Launch();
    }
    User::LeaveIfError(err);
}

void CTgWorker::DoCancel()
{
    iThread.Kill(KErrCancel);
    iThread.Close();
    TRequestStatus *s = &iStatus;
    User::RequestComplete(s, KErrCancel);
}

TInt CTgWorker::ThreadMain(TAny *aSelf)
{
    CTrapCleanup *cleanup = CTrapCleanup::New();
    static_cast<CTgWorker *>(aSelf)->Loop();
    delete cleanup;
    return 0;
}

static const int KCannotOpen = -100;

static int ReadFile(void *aCtx, unsigned char *aBuf, int64_t aOffset, int aLen)
{
    TPtr8 p(aBuf, 0, aLen);
    if (static_cast<RFile *>(aCtx)->Read(TInt(aOffset), p, aLen) != KErrNone)
        return -1;
    return p.Length();
}

static int SendFile(tg_client *aClient, TTgJob &j, int aIndex)
{
    RFs fs;
    RFile f;
    TInt size = 0, rc;
    const TDesC &path = aIndex < 0 ? j.iSendPath : j.iAlbumPaths[aIndex];
    if (fs.Connect() != KErrNone)
        return KCannotOpen;
    if (f.Open(fs, path, EFileRead | EFileShareReadersOnly) != KErrNone) {
        fs.Close();
        return KCannotOpen;
    }
    f.Size(size);
    TBuf8<256> name;
    CnvUtfConverter::ConvertFromUnicodeToUtf8(name, TParsePtrC(path).NameAndExt());
    name.ZeroTerminate();
    if (aIndex < 0)
        rc = tg_client_send_file(aClient, &j.iPeer, ReadFile, &f, size, (const char *)name.Ptr(),
                                 j.iSendAsPhoto, j.iText, &j.iProgress);
    else
        rc = tg_client_upload_photo(aClient, &j.iPeer, ReadFile, &f, size,
                                    (const char *)name.Ptr(), &j.iAlbumLocs[aIndex],
                                    &j.iProgress);
    f.Close();
    fs.Close();
    return rc;
}

static int SendAlbum(tg_client *aClient, TTgJob &j)
{
    int i, rc = TG_OK;
    for (i = 0; i < j.iAlbumCount && rc == TG_OK; i++) {
        j.iAlbumIndex = i;
        j.iProgress = 0;
        rc = SendFile(aClient, j, i);
    }
    if (rc == TG_OK)
        rc = tg_client_send_album(aClient, &j.iPeer, j.iAlbumLocs, j.iAlbumCount, j.iText);
    return rc;
}

void CTgWorker::RunJob(tg_client *aClient)
{
    TTgJob &j = iJob;
    if (!aClient) {
        j.iResult = TG_ERR_NOMEM;
        strcpy(j.iError, "could not start the Telegram client");
        return;
    }
    switch (j.iType) {
    case EJobDialogs:
        j.iDoneFolder = j.iFolder;
        if (j.iWithFolders)
            tg_client_folders(aClient, &j.iFolders, &j.iNumFolders);
        j.iResult = 0;
        if (j.iDoneFolder != 1)
            j.iResult = tg_client_dialogs(aClient, 0, 100, &j.iDialogs, &j.iNumDialogs);
        if (!j.iResult && j.iDoneFolder != 0)
            j.iResult = tg_client_dialogs(aClient, 1, 100, &j.iArchive, &j.iNumArchive);
        break;
    case EJobHistory:
        j.iResult = tg_client_history(aClient, &j.iPeer, j.iOffsetId, 30, &j.iMessages, &j.iNumMessages);
        {
            int32_t when = 0;
            j.iStatus = tg_client_status(aClient, &when);
            j.iStatusWhen = when;
        }
        free(j.iPinned);
        j.iPinned = NULL;
        if (!j.iResult && !j.iOffsetId)
            tg_client_pinned(aClient, &j.iPeer, &j.iPinned);
        j.iReadDone = 0;
        j.iReadOut = -1;
        if (!j.iResult) {
            int32_t ro;
            if (!tg_client_read_out(aClient, &j.iPeer, &ro))
                j.iReadOut = ro;
        }
        if (!j.iResult && j.iMarkRead && j.iNumMessages) {
            int newest = j.iMessages[j.iNumMessages - 1].id;
            if (newest > j.iReadAbove && !tg_client_read_history(aClient, &j.iPeer, newest))
                j.iReadDone = newest;
        }
        break;
    case EJobSend:
        j.iResult = tg_client_send(aClient, &j.iPeer, j.iText, j.iReplyTo);
        break;
    case EJobSendCode:
        j.iResult = tg_client_send_code(aClient, j.iText, j.iCodeType, sizeof j.iCodeType);
        break;
    case EJobResendCode:
        j.iResult = tg_client_resend_code(aClient, j.iCodeType, sizeof j.iCodeType);
        break;
    case EJobQrLogin:
        j.iResult = tg_client_qr_login(aClient, j.iText, sizeof j.iText);
        break;
    case EJobAvatars:
        j.iResult = 0;
        for (int i = 0; i < j.iNumLocs; i++)
            j.iLocOk[i] = tg_client_download(aClient, &j.iLocs[i], j.iPaths[i]) == 0;
        break;
    case EJobChatAction:
        j.iResult = tg_client_chat_action(aClient, &j.iPeer, j.iMsgId);
        break;
    case EJobInfo:
        j.iResult = tg_client_info(aClient, &j.iPeer, &j.iInfo);
        break;
    case EJobContacts:
        j.iResult = tg_client_contacts(aClient, &j.iContacts, &j.iNumContacts);
        break;
    case EJobResolve:
        j.iResult = tg_client_resolve(aClient, j.iText, &j.iResolved);
        break;
    case EJobPwHint:
        j.iResult = tg_client_password_hint(aClient, j.iCodeType, sizeof j.iCodeType);
        break;
    case EJobPassword:
        j.iResult = tg_client_check_password(aClient, j.iText);
        memset(j.iText, 0, sizeof j.iText);
        break;
    case EJobEdit:
        j.iResult = tg_client_edit(aClient, &j.iPeer, j.iMsgId, j.iText);
        break;
    case EJobDelete:
        j.iResult = tg_client_delete(aClient, &j.iPeer, j.iIds, j.iNumIds, j.iRevoke);
        break;
    case EJobForward:
        j.iResult = tg_client_forward(aClient, &j.iPeer, j.iMsgId, &j.iTarget);
        break;
    case EJobReact:
        j.iResult = tg_client_react(aClient, &j.iPeer, j.iMsgId, j.iText[0] ? j.iText : NULL);
        break;
    case EJobSendFile:
        j.iProgress = 0;
        j.iAlbumIndex = 0;
        j.iResult = j.iAlbumCount ? SendAlbum(aClient, j) : SendFile(aClient, j, -1);
        if (j.iResult == KCannotOpen) {
            j.iResult = TG_ERR_PROTO;
            strcpy(j.iError, "cannot open the file");
            return;
        }
        break;
    case EJobMedia:
        j.iResult = tg_client_download(aClient, &j.iMediaLoc, j.iMediaPath);
        break;
    case EJobPoll:
        j.iResult = tg_client_poll(aClient, &j.iChanged);
        break;
    case EJobSignIn:
        j.iResult = tg_client_sign_in(aClient, j.iText);
        break;
    case EJobLogout:
        j.iResult = tg_client_logout(aClient);
        break;
    }
    if (j.iResult) {
        strncpy(j.iError, tg_client_error(aClient), sizeof j.iError - 1);
        j.iError[sizeof j.iError - 1] = 0;
    }
}

void CTgWorker::Loop()
{
    tg_client *client = tg_client_new(iDir, TG_API_ID, TG_API_HASH, 0);
    for (;;) {
        iWake.Wait();
        if (iQuit)
            break;
        RunJob(client);
        TRequestStatus *status = iUiStatus;
        RThread ui;
        if (ui.Open(iUiThread) == KErrNone) {
            ui.RequestComplete(status, KErrNone);
            ui.Close();
        }
    }
    tg_client_free(client);
}
