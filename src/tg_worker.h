#ifndef TG_WORKER_H
#define TG_WORKER_H

#include <e32base.h>
#include "tg_client.h"

enum TTgJobType
    {
    EJobDialogs,
    EJobHistory,
    EJobSend,
    EJobSendCode,
    EJobResendCode,
    EJobQrLogin,
    EJobPoll,
    EJobAvatars,
    EJobMedia,
    EJobSendFile,
    EJobEdit,
    EJobDelete,
    EJobForward,
    EJobReact,
    EJobChatAction,
    EJobInfo,
    EJobContacts,
    EJobResolve,
    EJobPwHint,
    EJobPassword,
    EJobSignIn,
    EJobLogout
    };

struct TTgJob
    {
    TTgJobType iType;
    tg_dialog iPeer;
    char iText[2048];
    int iResult;
    int iChanged;
    char iError[200];
    char iCodeType[128];
    int iFolder;
    int iWithFolders;
    int iDoneFolder;
    tg_dialog *iDialogs;
    int iNumDialogs;
    tg_folder *iFolders;
    int iNumFolders;
    tg_dialog *iArchive;
    int iNumArchive;
    tg_file_loc iLocs[4];
    char iPaths[4][128];
    int iLocOk[4];
    int iNumLocs;
    int iMarkRead;
    int iReadAbove;
    int iReadDone;
    int iReadOut;
    int iOffsetId;
    int iReplyTo;
    int iMsgId;
    int iStatus;
    int iStatusWhen;
    tg_dialog iTarget;
    char *iPinned;
    tg_info iInfo;
    tg_dialog *iContacts;
    int iNumContacts;
    tg_dialog iResolved;
    TFileName iSendPath;
    int iSendAsPhoto;
    volatile int iProgress;
    TFileName iAlbumPaths[10];
    tg_file_loc iAlbumLocs[10];
    int iAlbumCount;
    int32_t iIds[10];
    int iNumIds;
    int iRevoke;
    volatile int iAlbumIndex;
    tg_file_loc iMediaLoc;
    int iMediaPurpose;
    char iMediaPath[128];
    tg_message *iMessages;
    int iNumMessages;
    };

class MTgObserver
{
public:
    virtual void JobDoneL(TTgJob &aJob) = 0;
};

class CTgWorker : public CActive
{
public:
    static CTgWorker *NewL(MTgObserver &aObserver, const char *aDir);
    ~CTgWorker();
    TBool Busy() const { return IsActive() || iDeferred; }
    TTgJob &Job() { return iNext; }
    const TTgJob &Running() const { return iJob; }
    TInt Start();
    void ClearOutputs();

private:
    CTgWorker(MTgObserver &aObserver);
    void ConstructL(const char *aDir);
    void RunL();
    void DoCancel();
    static TInt ThreadMain(TAny *aSelf);
    void Loop();
    void RunJob(tg_client *aClient);
    void Launch();

    MTgObserver &iObserver;
    RThread iThread;
    RSemaphore iWake;
    TThreadId iUiThread;
    TRequestStatus *iUiStatus;
    TBool iQuit;
    char iDir[128];
    TBool iInDone;
    TBool iDeferred;
    TTgJob iJob;
    TTgJob iNext;
};

#endif
