#include <aknapp.h>
#include <apgtask.h>
#include <apgcli.h>
#include <akndoc.h>
#include <aknappui.h>
#include <akntitle.h>
#include <aknlists.h>
#include <aknquerydialog.h>
#include <aknlistquerydialog.h>
#include <aknmessagequerydialog.h>
#include <aknnavi.h>
#include <aknglobalnote.h>
#include <caknfileselectiondialog.h>
#include <ecam.h>
#include <baclipb.h>
#include <txtetext.h>
#include <aknnotewrappers.h>
#include <aknkeylock.h>
#include <aknpopup.h>
#include <aknsfld.h>
#include <aknsdrawutils.h>
#include <aknsutils.h>
#include <aknsconstants.h>
#include <aknutils.h>
#include <documenthandler.h>
#include <aknquerycontrol.h>
#include <hwrmvibra.h>
#include <aknnavide.h>
#include <akntabgrp.h>
#include <e32math.h>
#include <aknpopupheadingpane.h>
#include <avkon.hrh>
#include <avkon.rsg>
#include <badesca.h>
#include <bitdev.h>
#include <bitstd.h>
#include <eikenv.h>
#include <eikimage.h>
#include <eiklbo.h>
#include <eikmenup.h>
#include <eikspane.h>
#include <eikbtgpc.h>
#include <eikstart.h>
#include <coecntrl.h>
#include <f32file.h>
#include <fbs.h>
#include <gulicon.h>
#include <imageconversion.h>
#include <utf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <tgnative.rsg>
#include "tgnative.hrh"
#include "tg_worker.h"
#include "tg_mtproto.h"
#include "tg_qr.h"
#include "rsym_tcp.h"

class CTgAppUi;

static TInt gLang;

struct TTgTr
    {
    const TText *iEn, *iRu, *iUk;
    };

static const TTgTr KTr[] = {
    {_S("Options"), _S("Функции"), _S("Функції")},
    {_S("Exit"), _S("Выход"), _S("Вихід")},
    {_S("Hide"), _S("Скрыть"), _S("Сховати")},
    {_S("Shortcuts"), _S("Горячие клавиши"), _S("Гарячі клавіші")},
    {_S("In a chat"), _S("В чате"), _S("У чаті")},
    {_S("In the chat list"), _S("В списке чатов"), _S("У списку чатів")},
    {_S("New message"), _S("Новое сообщение"), _S("Нове повідомлення")},
    {_S("Mute / unmute"), _S("Без звука / со звуком"), _S("Без звуку / зі звуком")},
    {_S("Pin / unpin"), _S("Закрепить / открепить"), _S("Закріпити / відкріпити")},
    {_S("Archive / unarchive"), _S("В архив / из архива"), _S("В архів / з архіву")},
    {_S("Read / unread"), _S("Прочитано / непрочитано"), _S("Прочитано / непрочитано")},
    {_S("Folders"), _S("Папки"), _S("Папки")},
    {_S("First / last message"), _S("Первое / последнее сообщение"), _S("Перше / останнє повідомлення")},
    {_S("Send photos"), _S("Отправить фото"), _S("Надіслати фото")},
    {_S("Open link"), _S("Открыть ссылку"), _S("Відкрити посилання")},
    {_S("Choose a link"), _S("Выберите ссылку"), _S("Виберіть посилання")},
    {_S("Download"), _S("Скачать"), _S("Завантажити")},
    {_S("Back"), _S("Назад"), _S("Назад")},
    {_S("Reply"), _S("Ответить"), _S("Відповісти")},
    {_S("Photo"), _S("Фото"), _S("Фото")},
    {_S("Preview"), _S("Просмотр"), _S("Перегляд")},
    {_S("Capture"), _S("Снять"), _S("Зняти")},
    {_S("Open"), _S("Открыть"), _S("Відкрити")},
    {_S("Chat"), _S("Чат"), _S("Чат")},
    {_S("Attach"), _S("Вложение"), _S("Вкладення")},
    {_S("Message"), _S("Сообщение"), _S("Повідомлення")},
    {_S("Pinned message"), _S("Закреплённое сообщение"), _S("Закріплене повідомлення")},
    {_S("Info"), _S("Информация"), _S("Інформація")},
    {_S("Contacts"), _S("Контакты"), _S("Контакти")},
    {_S("Find by username"), _S("Найти по имени пользователя"), _S("Знайти за іменем користувача")},
    {_S("Load older messages"), _S("Загрузить старые сообщения"), _S("Завантажити старіші повідомлення")},
    {_S("Message text"), _S("Текст сообщения"), _S("Текст повідомлення")},
    {_S("Zoom in"), _S("Увеличить"), _S("Збільшити")},
    {_S("Zoom out"), _S("Уменьшить"), _S("Зменшити")},
    {_S("Save to memory card"), _S("Сохранить на карту памяти"), _S("Зберегти на карту пам'яті")},
    {_S("Refresh"), _S("Обновить"), _S("Оновити")},
    {_S("Log in with QR code"), _S("Войти по QR-коду"), _S("Увійти за QR-кодом")},
    {_S("Log in with phone number"), _S("Войти по номеру телефона"), _S("Увійти за номером телефону")},
    {_S("Log out"), _S("Выйти из аккаунта"), _S("Вийти з акаунта")},
    {_S("Diagnostics"), _S("Диагностика"), _S("Діагностика")},
    {_S("Language"), _S("Язык"), _S("Мова")},
    {_S("Automatic"), _S("Автоматически"), _S("Автоматично")},
    {_S("Pin"), _S("Закрепить"), _S("Закріпити")},
    {_S("Unpin"), _S("Открепить"), _S("Відкріпити")},
    {_S("Mute"), _S("Без звука"), _S("Без звуку")},
    {_S("Unmute"), _S("Включить звук"), _S("Увімкнути звук")},
    {_S("Mark as read"), _S("Отметить как прочитанное"), _S("Позначити як прочитане")},
    {_S("Mark as unread"), _S("Отметить как непрочитанное"), _S("Позначити як непрочитане")},
    {_S("Archive chat"), _S("В архив"), _S("До архіву")},
    {_S("Unarchive chat"), _S("Из архива"), _S("З архіву")},
    {_S("Reply to"), _S("Ответить на"), _S("Відповісти на")},
    {_S("Copy text"), _S("Копировать текст"), _S("Копіювати текст")},
    {_S("Edit"), _S("Изменить"), _S("Редагувати")},
    {_S("Delete"), _S("Удалить"), _S("Видалити")},
    {_S("Forward"), _S("Переслать"), _S("Переслати")},
    {_S("React"), _S("Реакция"), _S("Реакція")},
    {_S("Open in player"), _S("Открыть в плеере"), _S("Відкрити в плеєрі")},
    {_S("File"), _S("Файл"), _S("Файл")},
    {_S("Camera"), _S("Камера"), _S("Камера")},
    {_S("Add a caption?"), _S("Добавить подпись?"), _S("Додати підпис?")},
    {_S("All chats"), _S("Все чаты"), _S("Усі чати")},
    {_S("Archive"), _S("Архив"), _S("Архів")},
    {_S("Camera not available (%d)"), _S("Камера недоступна (%d)"), _S("Камера недоступна (%d)")},
    {_S("Caption"), _S("Подпись"), _S("Підпис")},
    {_S("Checking password...\nThis takes a few seconds"), _S("Проверка пароля...\nЭто займёт несколько секунд"), _S("Перевірка пароля...\nЦе займе кілька секунд")},
    {_S("Cloud password"), _S("Облачный пароль"), _S("Хмарний пароль")},
    {_S(" (hint: "), _S(" (подсказка: "), _S(" (підказка: ")},
    {_S("Code sent by SMS"), _S("Код отправлен по SMS"), _S("Код надіслано в SMS")},
    {_S("Code sent to your other Telegram app"), _S("Код отправлен в другое приложение Telegram"), _S("Код надіслано в інший застосунок Telegram")},
    {_S("Connecting..."), _S("Подключение..."), _S("Підключення...")},
    {_S("Copied"), _S("Скопировано"), _S("Скопійовано")},
    {_S("Could not load chats\nOptions > Refresh"), _S("Не удалось загрузить чаты\nФункции > Обновить"), _S("Не вдалося завантажити чати\nФункції > Оновити")},
    {_S("Could not open this picture"), _S("Не удалось открыть изображение"), _S("Не вдалося відкрити зображення")},
    {_S("Could not save to "), _S("Не удалось сохранить в "), _S("Не вдалося зберегти в ")},
    {_S("Delete this message?"), _S("Удалить это сообщение?"), _S("Видалити це повідомлення?")},
    {_S("Delete for everyone"), _S("Удалить у всех"), _S("Видалити для всіх")},
    {_S("Delete for me"), _S("Удалить у меня"), _S("Видалити для мене")},
    {_S("Edit message"), _S("Изменить сообщение"), _S("Редагувати повідомлення")},
    {_S("Error: "), _S("Ошибка: "), _S("Помилка: ")},
    {_S("Forward to"), _S("Переслать в"), _S("Переслати до")},
    {_S("Forwarded from "), _S("Переслано от "), _S("Переслано від ")},
    {_S("Forwarded"), _S("Переслано"), _S("Переслано")},
    {_S(" \x00B7 Fwd: "), _S(" \x00B7 Пересл.: "), _S(" \x00B7 Пересл.: ")},
    {_S(" \x00B7 You"), _S(" \x00B7 Вы"), _S(" \x00B7 Ви")},
    {_S(" \x00B7 edited"), _S(" \x00B7 изм."), _S(" \x00B7 ред.")},
    {_S("Getting a QR code..."), _S("Получение QR-кода..."), _S("Отримання QR-коду...")},
    {_S("In reply to "), _S("В ответ на "), _S("У відповідь на ")},
    {_S("Loading full size..."), _S("Загрузка оригинала..."), _S("Завантаження оригіналу...")},
    {_S("Loading older..."), _S("Загрузка старых..."), _S("Завантаження старіших...")},
    {_S("Loading picture..."), _S("Загрузка изображения..."), _S("Завантаження зображення...")},
    {_S("Loading..."), _S("Загрузка..."), _S("Завантаження...")},
    {_S("Log out of this account?"), _S("Выйти из этого аккаунта?"), _S("Вийти з цього акаунта?")},
    {_S("Logged in\nLoading chats..."), _S("Вход выполнен\nЗагрузка чатов..."), _S("Вхід виконано\nЗавантаження чатів...")},
    {_S("Logging out..."), _S("Выход..."), _S("Вихід...")},
    {_S("Login code"), _S("Код входа"), _S("Код входу")},
    {_S("No app can open this file (%d)"), _S("Нет приложения для этого файла (%d)"), _S("Немає застосунку для цього файлу (%d)")},
    {_S("No chats"), _S("Нет чатов"), _S("Немає чатів")},
    {_S("No code? Ask Telegram to send it again?"), _S("Нет кода? Запросить ещё раз?"), _S("Немає коду? Запросити ще раз?")},
    {_S("No details"), _S("Нет данных"), _S("Немає даних")},
    {_S("No messages\nPress R to write"), _S("Нет сообщений\nНажмите R, чтобы написать"), _S("Немає повідомлень\nНатисніть R, щоб написати")},
    {_S("No older messages"), _S("Больше сообщений нет"), _S("Більше повідомлень немає")},
    {_S("Not logged in\nPress the select key to log in with a QR code"), _S("Вход не выполнен\nНажмите центральную клавишу для входа по QR-коду"), _S("Вхід не виконано\nНатисніть центральну клавішу для входу за QR-кодом")},
    {_S("Nothing to save in this message"), _S("В этом сообщении нечего сохранять"), _S("У цьому повідомленні нічого зберігати")},
    {_S("Remove my reaction"), _S("Убрать мою реакцию"), _S("Прибрати мою реакцію")},
    {_S("<3  Heart"), _S("<3  Сердце"), _S("<3  Серце")},
    {_S("+1  Like"), _S("+1  Нравится"), _S("+1  Подобається")},
    {_S("-1  Dislike"), _S("-1  Не нравится"), _S("-1  Не подобається")},
    {_S(":'D  Laugh"), _S(":'D  Смех"), _S(":'D  Сміх")},
    {_S(":O  Wow"), _S(":O  Ух ты"), _S(":O  Ого")},
    {_S(":'(  Sad"), _S(":'(  Грусть"), _S(":'(  Сум")},
    {_S("Fire"), _S("Огонь"), _S("Вогонь")},
    {_S("yesterday"), _S("вчера"), _S("учора")},
    {_S("Album (%d)"), _S("Альбом (%d)"), _S("Альбом (%d)")},
    {_S("Files (%d)"), _S("Файлы (%d)"), _S("Файли (%d)")},
    {_S("Photo %d"), _S("Фото %d"), _S("Фото %d")},
    {_S("Choose a file"), _S("Выберите файл"), _S("Виберіть файл")},
    {_S("Audio (%d)"), _S("Аудио (%d)"), _S("Аудіо (%d)")},
    {_S("Heart"), _S("Сердце"), _S("Серце")},
    {_S("Like"), _S("Нравится"), _S("Подобається")},
    {_S("Dislike"), _S("Не нравится"), _S("Не подобається")},
    {_S("Love"), _S("Любовь"), _S("Любов")},
    {_S("Clap"), _S("Аплодисменты"), _S("Оплески")},
    {_S("Grin"), _S("Улыбка"), _S("Усмішка")},
    {_S("Hmm"), _S("Хм"), _S("Хм")},
    {_S("Mind blown"), _S("Взрыв мозга"), _S("Вибух мозку")},
    {_S("Shock"), _S("Шок"), _S("Шок")},
    {_S("Sad"), _S("Грусть"), _S("Сум")},
    {_S("Party"), _S("Праздник"), _S("Свято")},
    {_S("Star-struck"), _S("Восторг"), _S("Захват")},
    {_S("Thanks"), _S("Спасибо"), _S("Дякую")},
    {_S("OK"), _S("ОК"), _S("ОК")},
    {_S("Laugh"), _S("Смех"), _S("Сміх")},
    {_S("Wow"), _S("Ух ты"), _S("Ого")},
    {_S("Adore"), _S("Обожаю"), _S("Обожнюю")},
    {_S("Cry"), _S("Плач"), _S("Плач")},
    {_S("Angry"), _S("Злость"), _S("Злість")},
    {_S("Sick"), _S("Тошнит"), _S("Нудить")},
    {_S("Poop"), _S("Какашка"), _S("Какашка")},
    {_S("Requesting a login code..."), _S("Запрос кода входа..."), _S("Запит коду входу...")},
    {_S("Requesting the code again..."), _S("Повторный запрос кода..."), _S("Повторний запит коду...")},
    {_S("Saved to "), _S("Сохранено в "), _S("Збережено в ")},
    {_S("Saving..."), _S("Сохранение..."), _S("Збереження...")},
    {_S("Scan to log in"), _S("Сканируйте для входа"), _S("Скануйте для входу")},
    {_S("Select file"), _S("Выберите файл"), _S("Виберіть файл")},
    {_S("Select photo"), _S("Выберите фото"), _S("Виберіть фото")},
    {_S("Select photos"), _S("Выберите фото"), _S("Виберіть фото")},
    {_S("Up to 10 photos per album, sending the first 10"),
     _S("В альбоме до 10 фото, отправляю первые 10"),
     _S("В альбомі до 10 фото, надсилаю перші 10")},
    {_S("Sending %d%%"), _S("Отправка %d%%"), _S("Надсилання %d%%")},
    {_S("Set up a login email in the official app first. Code"), _S("Сначала укажите почту для входа в официальном приложении. Код"), _S("Спершу вкажіть пошту для входу в офіційному застосунку. Код")},
    {_S("Signing in..."), _S("Вход..."), _S("Вхід...")},
    {_S("Today"), _S("Сегодня"), _S("Сьогодні")},
    {_S("Yesterday"), _S("Вчера"), _S("Учора")},
    {_S("Two-step verification"), _S("Двухэтапная проверка"), _S("Двоетапна перевірка")},
    {_S("Username, e.g. @durov"), _S("Имя пользователя, напр. @durov"), _S("Ім'я користувача, напр. @durov")},
    {_S("Waiting for the code\nTelegram says: "), _S("Ожидание кода\nTelegram: "), _S("Очікування коду\nTelegram: ")},
    {_S("You: "), _S("Вы: "), _S("Ви: ")},
    {_S("Your phone number, e.g. +380..."), _S("Ваш номер телефона, напр. +380..."), _S("Ваш номер телефону, напр. +380...")},
    {_S("last seen "), _S("был(а) "), _S("був(ла) ")},
    {_S("last seen recently"), _S("был(а) недавно"), _S("був(ла) нещодавно")},
    {_S("last seen within a month"), _S("был(а) в этом месяце"), _S("був(ла) цього місяця")},
    {_S("last seen within a week"), _S("был(а) на этой неделе"), _S("був(ла) цього тижня")},
    {_S("online"), _S("в сети"), _S("в мережі")},
    {_S("%d new"), _S("Новых: %d"), _S("Нових: %d")},
    {_S("%d members\n"), _S("Участников: %d\n"), _S("Учасників: %d\n")},
    {_S("%d subscribers\n"), _S("Подписчиков: %d\n"), _S("Підписників: %d\n")},
    {_S("%d chats with new messages: "), _S("Чатов с новыми сообщениями: %d \x2014 "), _S("Чатів з новими повідомленнями: %d \x2014 ")},
    {_S("On your main phone"), _S("На основном телефоне"), _S("На основному телефоні")},
    {_S("open Telegram:"), _S("откройте Telegram:"), _S("відкрийте Telegram:")},
    {_S("Settings >"), _S("Настройки >"), _S("Налаштування >")},
    {_S("Devices >"), _S("Устройства >"), _S("Пристрої >")},
    {_S("Link Desktop"), _S("Подключить"), _S("Підключити")},
    {_S("Device"), _S("устройство"), _S("пристрій")},
    {_S("and scan this code."), _S("и отсканируйте код."), _S("і відскануйте код.")},
    {_S("Jan"), _S("янв"), _S("січ")},
    {_S("Feb"), _S("фев"), _S("лют")},
    {_S("Mar"), _S("мар"), _S("бер")},
    {_S("Apr"), _S("апр"), _S("кві")},
    {_S("May"), _S("мая"), _S("тра")},
    {_S("Jun"), _S("июн"), _S("чер")},
    {_S("Jul"), _S("июл"), _S("лип")},
    {_S("Aug"), _S("авг"), _S("сер")},
    {_S("Sep"), _S("сен"), _S("вер")},
    {_S("Oct"), _S("окт"), _S("жов")},
    {_S("Nov"), _S("ноя"), _S("лис")},
    {_S("Dec"), _S("дек"), _S("гру")}
};

static const struct {
    TUint32 iCp;
    const TText *iName;
} KEmoji[] = {
    {0x2764, _S("Heart")},       {0x1f44d, _S("Like")},       {0x1f44e, _S("Dislike")},
    {0x1f525, _S("Fire")},       {0x1f970, _S("Love")},       {0x1f44f, _S("Clap")},
    {0x1f601, _S("Grin")},       {0x1f914, _S("Hmm")},        {0x1f92f, _S("Mind blown")},
    {0x1f631, _S("Shock")},      {0x1f622, _S("Sad")},        {0x1f389, _S("Party")},
    {0x1f929, _S("Star-struck")}, {0x1f64f, _S("Thanks")},    {0x1f44c, _S("OK")},
    {0x1f602, _S("Laugh")},      {0x1f62e, _S("Wow")},        {0x1f60d, _S("Adore")},
    {0x1f62d, _S("Cry")},        {0x1f92c, _S("Angry")},      {0x1f92e, _S("Sick")},
    {0x1f4a9, _S("Poop")}};

static TPtrC T(const TDesC &aEn)
{
    if (gLang)
        for (TUint i = 0; i < sizeof KTr / sizeof KTr[0]; i++)
            if (aEn == TPtrC(KTr[i].iEn))
                return TPtrC(gLang == 1 ? KTr[i].iRu : KTr[i].iUk);
    return TPtrC(aEn.Ptr(), aEn.Length());
}

static HBufC *TextLC(const char *aText, TInt aLen = -1)
{
    TInt len = aText ? (aLen >= 0 ? aLen : User::StringLength((const TUint8 *)aText)) : 0;
    TPtrC8 in((const TUint8 *)(aText ? aText : ""), len);
    HBufC *out = HBufC::NewLC(len + 1);
    TPtr p = out->Des();
    if (CnvUtfConverter::ConvertToUnicodeFromUtf8(p, in) < 0)
        p.Copy(in);
    return out;
}

static void ToUtf8(char *aDst, int aSize, const TDesC &aText)
{
    HBufC8 *u = HBufC8::NewLC(aText.Length() * 3 + 1);
    TPtr8 p = u->Des();
    CnvUtfConverter::ConvertFromUnicodeToUtf8(p, aText);
    int n = p.Length() < aSize - 1 ? p.Length() : aSize - 1;
    memcpy(aDst, p.Ptr(), n);
    aDst[n] = 0;
    CleanupStack::PopAndDestroy(u);
}

static void InfoL(const TDesC &aText)
{
    CAknQueryDialog *dlg = CAknQueryDialog::NewL();
    dlg->ExecuteLD(R_TG_OK_QUERY, aText);
}

static TInt PickL(const TDesC &aTitle, MDesCArray *aItems)
{
    CAknSinglePopupMenuStyleListBox *lb = new (ELeave) CAknSinglePopupMenuStyleListBox;
    CleanupStack::PushL(lb);
    CAknPopupList *popup =
        CAknPopupList::NewL(lb, R_AVKON_SOFTKEYS_OK_CANCEL, AknPopupLayouts::EMenuWindow);
    CleanupStack::PushL(popup);
    lb->ConstructL(popup, EAknListBoxSelectionList | EAknListBoxLoopScrolling);
    lb->CreateScrollBarFrameL(ETrue);
    lb->ScrollBarFrame()->SetScrollBarVisibilityL(CEikScrollBarFrame::EOff,
                                                  CEikScrollBarFrame::EAuto);
    lb->Model()->SetItemTextArray(aItems);
    lb->Model()->SetOwnershipType(ELbmDoesNotOwnItemArray);
    popup->SetTitleL(aTitle);
    popup->EnableFind();
    CleanupStack::Pop(popup);
    TInt index = -1;
    if (popup->ExecuteLD()) {
        index = lb->CurrentItemIndex();
        CAknListBoxFilterItems *filter =
            static_cast<CAknFilteredTextListBoxModel *>(lb->Model())->Filter();
        if (filter && index >= 0)
            index = filter->FilteredItemIndex(index);
    }
    CleanupStack::PopAndDestroy(lb);
    return index;
}

static void FindLinksL(const TDesC &aText, CDesCArray &aOut)
{
    _LIT(KHttp, "http://");
    _LIT(KHttps, "https://");
    _LIT(KWww, "www.");
    _LIT(KTme, "t.me/");
    for (TInt i = 0; i < aText.Length(); i++) {
        if (i > 0 && !TChar(aText[i - 1]).IsSpace() && aText[i - 1] != '(')
            continue;
        TPtrC rest = aText.Mid(i);
        TBool scheme = rest.Left(7).CompareF(KHttp) == 0 || rest.Left(8).CompareF(KHttps) == 0;
        if (!scheme && rest.Left(4).CompareF(KWww) && rest.Left(5).CompareF(KTme))
            continue;
        TInt end = 0;
        while (end < rest.Length() && !TChar(rest[end]).IsSpace())
            end++;
        while (end > 0 && TPtrC(_L(".,;:!?)]\"'")).Locate(rest[end - 1]) >= 0)
            end--;
        HBufC *url = HBufC::NewLC(end + 8);
        if (!scheme)
            url->Des().Append(KHttps);
        url->Des().Append(rest.Left(end));
        aOut.AppendL(*url);
        CleanupStack::PopAndDestroy(url);
        i += end;
    }
}

static void OpenUrlL(const TDesC &aUrl)
{
    const TUid KBrowser = {0x10008D39};
    HBufC *param = HBufC::NewLC(aUrl.Length() + 2);
    param->Des().Append(_L("4 "));
    param->Des().Append(aUrl);
    TApaTask task = TApaTaskList(CEikonEnv::Static()->WsSession()).FindApp(KBrowser);
    if (task.Exists()) {
        HBufC8 *p8 = CnvUtfConverter::ConvertFromUnicodeToUtf8L(*param);
        CleanupStack::PushL(p8);
        task.SendMessage(TUid::Uid(0), *p8);
        task.BringToForeground();
        CleanupStack::PopAndDestroy(p8);
    } else {
        RApaLsSession ls;
        User::LeaveIfError(ls.Connect());
        CleanupClosePushL(ls);
        TThreadId id;
        User::LeaveIfError(ls.StartDocument(*param, KBrowser, id));
        CleanupStack::PopAndDestroy(&ls);
    }
    CleanupStack::PopAndDestroy(param);
}

static TBool AskYesNoL(const TDesC &aPrompt)
{
    CAknQueryDialog *dlg = CAknQueryDialog::NewL();
    return dlg->ExecuteLD(R_TG_YESNO_QUERY, aPrompt) != 0;
}

class CTgTextQuery : public CAknTextQueryDialog
{
public:
    CTgTextQuery(TDes &aBuf, TDes *aCancelled) : CAknTextQueryDialog(aBuf), iCancelled(aCancelled) {}

protected:
    TBool OkToExitL(TInt aButton)
    {
        if (iCancelled && aButton == EAknSoftkeyCancel && QueryControl())
            QueryControl()->GetText(*iCancelled);
        return CAknTextQueryDialog::OkToExitL(aButton);
    }

private:
    TDes *iCancelled;
};

static TBool QueryUtf8L(const TDesC &aPrompt, char *aOut, int aSize, TInt aResource,
                        const TDesC &aInitial = KNullDesC, TDes *aCancelled = NULL)
{
    HBufC *buf = HBufC::NewLC(1000);
    TPtr p = buf->Des();
    p.Copy(aInitial.Left(p.MaxLength()));
    CTgTextQuery *dlg = new (ELeave) CTgTextQuery(p, aCancelled);
    dlg->SetPromptL(aPrompt);
    dlg->SetMaxLength(p.MaxLength());
    TBool ok = dlg->ExecuteLD(aResource) != 0;
    if (ok) {
        p.Trim();
        ok = p.Length() > 0;
        if (ok)
            ToUtf8(aOut, aSize, p);
    }
    CleanupStack::PopAndDestroy(buf);
    return ok;
}

static void DayText(int32_t aDate, TDes &aOut)
{
    static const TText *const months[] = {_S("Jan"), _S("Feb"), _S("Mar"), _S("Apr"),
                                          _S("May"), _S("Jun"), _S("Jul"), _S("Aug"),
                                          _S("Sep"), _S("Oct"), _S("Nov"), _S("Dec")};
    TInt off = User::UTCOffset().Int();
    TTime now;
    now.UniversalTime();
    TTimeIntervalSeconds secs;
    now.SecondsFrom(TTime(_L("19700000:")), secs);
    TInt today = (secs.Int() + off) / 86400, day = (aDate + off) / 86400;
    if (day == today) {
        aOut.Copy(T(_L("Today")));
    } else if (day == today - 1) {
        aOut.Copy(T(_L("Yesterday")));
    } else {
        TTime t(_L("19700000:"));
        t += TTimeIntervalSeconds(aDate + off);
        TDateTime dt = t.DateTime();
        aOut.Num(dt.Day() + 1);
        aOut.Append(' ');
        aOut.Append(T(TPtrC(months[dt.Month()])));
    }
}

static TInt LocalDay(int32_t aDate) { return (aDate + User::UTCOffset().Int()) / 86400; }

static TInt NowUnix()
{
    TTime now;
    now.UniversalTime();
    TTimeIntervalSeconds secs;
    now.SecondsFrom(TTime(_L("19700000:")), secs);
    return secs.Int();
}

static void TimeText(int32_t aDate, TDes &aOut)
{
    TInt local = aDate + User::UTCOffset().Int();
    aOut.Format(_L("%02d:%02d"), (local / 3600) % 24, (local / 60) % 60);
}

static CFbsBitmap *RoundMaskLC(const TSize &aSize)
{
    CFbsBitmap *mask = new (ELeave) CFbsBitmap;
    CleanupStack::PushL(mask);
    User::LeaveIfError(mask->Create(aSize, EGray256));
    HBufC8 *line = HBufC8::NewLC(aSize.iWidth + 4);
    TPtr8 lp = line->Des();
    TReal r = Min(aSize.iWidth, aSize.iHeight) / 2.0;
    TReal cx = aSize.iWidth / 2.0, cy = aSize.iHeight / 2.0;
    for (TInt y = 0; y < aSize.iHeight; y++) {
        lp.Zero();
        for (TInt x = 0; x < aSize.iWidth; x++) {
            TReal dx = x + 0.5 - cx, dy = y + 0.5 - cy, d;
            Math::Sqrt(d, dx * dx + dy * dy);
            TReal a = r - d + 0.5;
            lp.Append(a <= 0 ? 0 : a >= 1 ? 255 : TUint8(a * 255));
        }
        mask->SetScanLine(lp, y);
    }
    CleanupStack::PopAndDestroy(line);
    return mask;
}

static TInt CapsBaselineL(CFont *aFont, const TSize &aSize)
{
    static TInt cacheKey[8], cacheBase[8], cacheUsed;
    TInt key = aSize.iHeight << 16 | aFont->HeightInPixels();
    for (TInt i = 0; i < cacheUsed; i++)
        if (cacheKey[i] == key)
            return cacheBase[i];
    TInt base = (aSize.iHeight - aFont->HeightInPixels()) / 2 + aFont->AscentInPixels();
    CFbsBitmap *probe = new (ELeave) CFbsBitmap;
    CleanupStack::PushL(probe);
    User::LeaveIfError(probe->Create(aSize, EGray256));
    CFbsBitmapDevice *dev = CFbsBitmapDevice::NewL(probe);
    CleanupStack::PushL(dev);
    CFbsBitGc *gc = NULL;
    User::LeaveIfError(dev->CreateContext(gc));
    CleanupStack::PushL(gc);
    gc->SetBrushStyle(CGraphicsContext::ESolidBrush);
    gc->SetBrushColor(KRgbBlack);
    gc->Clear();
    gc->UseFont(aFont);
    gc->SetPenColor(KRgbWhite);
    gc->DrawText(_L("H"), TRect(aSize), base, CGraphicsContext::ECenter);
    gc->DiscardFont();
    TInt top = -1, bottom = -1;
    TBuf8<256> line;
    for (TInt y = 0; y < aSize.iHeight; y++) {
        probe->GetScanLine(line, TPoint(0, y), Min(aSize.iWidth, 256), EGray256);
        for (TInt x = 0; x < line.Length(); x++)
            if (line[x] > 128) {
                if (top < 0)
                    top = y;
                bottom = y;
                break;
            }
    }
    CleanupStack::PopAndDestroy(3);
    if (top >= 0)
        base += (aSize.iHeight - (bottom - top + 1)) / 2 - top;
    TInt slot = cacheUsed < 8 ? cacheUsed++ : 7;
    cacheKey[slot] = key;
    cacheBase[slot] = base;
    return base;
}

static CGulIcon *MuteIconL(const TSize &aSize)
{
    CFbsBitmap *bmp = new (ELeave) CFbsBitmap;
    CleanupStack::PushL(bmp);
    User::LeaveIfError(bmp->Create(aSize, EColor64K));
    CFbsBitmapDevice *dev = CFbsBitmapDevice::NewL(bmp);
    CleanupStack::PushL(dev);
    CFbsBitGc *gc = NULL;
    User::LeaveIfError(dev->CreateContext(gc));
    gc->SetPenStyle(CGraphicsContext::ENullPen);
    gc->SetBrushStyle(CGraphicsContext::ESolidBrush);
    gc->SetBrushColor(TRgb(0x58, 0x58, 0x58));
    gc->DrawRect(TRect(aSize));
    delete gc;
    CleanupStack::PopAndDestroy(dev);
    CFbsBitmap *mask = new (ELeave) CFbsBitmap;
    CleanupStack::PushL(mask);
    User::LeaveIfError(mask->Create(aSize, EGray256));
    dev = CFbsBitmapDevice::NewL(mask);
    CleanupStack::PushL(dev);
    User::LeaveIfError(dev->CreateContext(gc));
    CleanupStack::PushL(gc);
    gc->SetBrushStyle(CGraphicsContext::ESolidBrush);
    gc->SetBrushColor(KRgbBlack);
    gc->Clear();
    TInt sd = Min(aSize.iWidth, aSize.iHeight);
    TInt x = (aSize.iWidth - sd) / 2, y = (aSize.iHeight - sd) / 2;
    gc->SetPenStyle(CGraphicsContext::ENullPen);
    gc->SetBrushColor(KRgbWhite);
    TPoint body[6] = {TPoint(x + sd * 10 / 100, y + sd * 38 / 100), TPoint(x + sd * 30 / 100, y + sd * 38 / 100),
                      TPoint(x + sd * 55 / 100, y + sd * 15 / 100), TPoint(x + sd * 55 / 100, y + sd * 85 / 100),
                      TPoint(x + sd * 30 / 100, y + sd * 62 / 100), TPoint(x + sd * 10 / 100, y + sd * 62 / 100)};
    gc->DrawPolygon(body, 6);
    gc->SetPenStyle(CGraphicsContext::ESolidPen);
    gc->SetPenColor(KRgbWhite);
    gc->SetPenSize(TSize(Max(1, sd / 9), Max(1, sd / 9)));
    gc->DrawLine(TPoint(x + sd * 66 / 100, y + sd * 35 / 100), TPoint(x + sd * 90 / 100, y + sd * 65 / 100));
    gc->DrawLine(TPoint(x + sd * 90 / 100, y + sd * 35 / 100), TPoint(x + sd * 66 / 100, y + sd * 65 / 100));
    CleanupStack::PopAndDestroy(2);
    CGulIcon *icon = CGulIcon::NewL(bmp, mask);
    CleanupStack::Pop(2);
    return icon;
}

static CGulIcon *CheckIconL(const TSize &aSize, TBool aDouble)
{
    CFbsBitmap *bmp = new (ELeave) CFbsBitmap;
    CleanupStack::PushL(bmp);
    User::LeaveIfError(bmp->Create(aSize, EColor64K));
    CFbsBitmapDevice *dev = CFbsBitmapDevice::NewL(bmp);
    CleanupStack::PushL(dev);
    CFbsBitGc *gc = NULL;
    User::LeaveIfError(dev->CreateContext(gc));
    gc->SetPenStyle(CGraphicsContext::ENullPen);
    gc->SetBrushStyle(CGraphicsContext::ESolidBrush);
    gc->SetBrushColor(TRgb(0x2E, 0x7D, 0xE8));
    gc->DrawRect(TRect(aSize));
    delete gc;
    CleanupStack::PopAndDestroy(dev);
    CFbsBitmap *mask = new (ELeave) CFbsBitmap;
    CleanupStack::PushL(mask);
    User::LeaveIfError(mask->Create(aSize, EGray256));
    dev = CFbsBitmapDevice::NewL(mask);
    CleanupStack::PushL(dev);
    User::LeaveIfError(dev->CreateContext(gc));
    CleanupStack::PushL(gc);
    gc->SetBrushStyle(CGraphicsContext::ESolidBrush);
    gc->SetBrushColor(KRgbBlack);
    gc->Clear();
    TInt sd = Min(aSize.iWidth, aSize.iHeight);
    TInt x = (aSize.iWidth - sd) / 2, y = (aSize.iHeight - sd) / 2;
    gc->SetPenStyle(CGraphicsContext::ESolidPen);
    gc->SetPenColor(KRgbWhite);
    gc->SetPenSize(TSize(Max(2, sd / 9), Max(2, sd / 9)));
    if (aDouble) {
        gc->DrawLine(TPoint(x + sd * 4 / 100, y + sd * 52 / 100),
                     TPoint(x + sd * 22 / 100, y + sd * 70 / 100));
        gc->DrawLine(TPoint(x + sd * 22 / 100, y + sd * 70 / 100),
                     TPoint(x + sd * 56 / 100, y + sd * 30 / 100));
        gc->DrawLine(TPoint(x + sd * 46 / 100, y + sd * 70 / 100),
                     TPoint(x + sd * 94 / 100, y + sd * 30 / 100));
    } else {
        gc->DrawLine(TPoint(x + sd * 18 / 100, y + sd * 50 / 100),
                     TPoint(x + sd * 38 / 100, y + sd * 70 / 100));
        gc->DrawLine(TPoint(x + sd * 38 / 100, y + sd * 70 / 100),
                     TPoint(x + sd * 82 / 100, y + sd * 28 / 100));
    }
    CleanupStack::PopAndDestroy(2);
    CGulIcon *icon = CGulIcon::NewL(bmp, mask);
    CleanupStack::Pop(2);
    return icon;
}

static CGulIcon *BadgeIconL(const TSize &aSize, TInt aCount, TBool aMuted)
{
    CFbsBitmap *bmp = new (ELeave) CFbsBitmap;
    CleanupStack::PushL(bmp);
    User::LeaveIfError(bmp->Create(aSize, EColor64K));
    CFbsBitmapDevice *dev = CFbsBitmapDevice::NewL(bmp);
    CleanupStack::PushL(dev);
    CFbsBitGc *gc = NULL;
    User::LeaveIfError(dev->CreateContext(gc));
    CleanupStack::PushL(gc);
    gc->SetPenStyle(CGraphicsContext::ENullPen);
    gc->SetBrushStyle(CGraphicsContext::ESolidBrush);
    gc->SetBrushColor(aMuted ? TRgb(0x30, 0x30, 0x30) : TRgb(0x1F, 0x5F, 0xC0));
    gc->DrawRect(TRect(aSize));
    gc->SetBrushColor(aMuted ? TRgb(0x66, 0x66, 0x66) : TRgb(0x3E, 0x88, 0xF7));
    gc->DrawEllipse(TRect(TPoint(1, 1), TSize(aSize.iWidth - 2, aSize.iHeight - 2)));
    TBuf<4> num;
    if (aCount > 99)
        num = _L("99+");
    else if (aCount > 0)
        num.Num(aCount);
    TFontSpec spec = CEikonEnv::Static()->TitleFont()->FontSpecInTwips();
    spec.iHeight = aSize.iHeight * (num.Length() > 2 ? 45 : num.Length() > 1 ? 58 : 70) / 100;
    spec.iFontStyle.SetStrokeWeight(EStrokeWeightBold);
    CFont *font = NULL;
    if (dev->GetNearestFontToDesignHeightInPixels(font, spec) == KErrNone) {
        gc->UseFont(font);
        gc->SetPenStyle(CGraphicsContext::ESolidPen);
        gc->SetPenColor(KRgbWhite);
        gc->SetBrushStyle(CGraphicsContext::ENullBrush);
        TInt base = CapsBaselineL(font, aSize);
        gc->DrawText(num, TRect(aSize), base, CGraphicsContext::ECenter);
        gc->DiscardFont();
        dev->ReleaseFont(font);
    }
    CleanupStack::PopAndDestroy(2);
    CFbsBitmap *mask = RoundMaskLC(aSize);
    CGulIcon *icon = CGulIcon::NewL(bmp, mask);
    CleanupStack::Pop(2);
    return icon;
}

static CGulIcon *PinIconL(const TSize &aSize)
{
    CFbsBitmap *bmp = new (ELeave) CFbsBitmap;
    CleanupStack::PushL(bmp);
    User::LeaveIfError(bmp->Create(aSize, EColor64K));
    CFbsBitmapDevice *dev = CFbsBitmapDevice::NewL(bmp);
    CleanupStack::PushL(dev);
    CFbsBitGc *gc = NULL;
    User::LeaveIfError(dev->CreateContext(gc));
    gc->SetPenStyle(CGraphicsContext::ENullPen);
    gc->SetBrushStyle(CGraphicsContext::ESolidBrush);
    gc->SetBrushColor(TRgb(0x58, 0x58, 0x58));
    gc->DrawRect(TRect(aSize));
    delete gc;
    CleanupStack::PopAndDestroy(dev);
    CFbsBitmap *mask = new (ELeave) CFbsBitmap;
    CleanupStack::PushL(mask);
    User::LeaveIfError(mask->Create(aSize, EGray256));
    dev = CFbsBitmapDevice::NewL(mask);
    CleanupStack::PushL(dev);
    User::LeaveIfError(dev->CreateContext(gc));
    CleanupStack::PushL(gc);
    gc->SetBrushStyle(CGraphicsContext::ESolidBrush);
    gc->SetBrushColor(KRgbBlack);
    gc->Clear();
    TInt sd = Min(aSize.iWidth, aSize.iHeight);
    TInt x = (aSize.iWidth - sd) / 2, y = (aSize.iHeight - sd) / 2;
    gc->SetPenStyle(CGraphicsContext::ENullPen);
    gc->SetBrushColor(KRgbWhite);
    TPoint head[4] = {TPoint(x + sd * 50 / 100, y + sd * 8 / 100), TPoint(x + sd * 92 / 100, y + sd * 50 / 100),
                      TPoint(x + sd * 62 / 100, y + sd * 62 / 100), TPoint(x + sd * 38 / 100, y + sd * 38 / 100)};
    gc->DrawPolygon(head, 4);
    TPoint base[4] = {TPoint(x + sd * 26 / 100, y + sd * 40 / 100), TPoint(x + sd * 60 / 100, y + sd * 74 / 100),
                      TPoint(x + sd * 52 / 100, y + sd * 82 / 100), TPoint(x + sd * 18 / 100, y + sd * 48 / 100)};
    gc->DrawPolygon(base, 4);
    gc->SetPenStyle(CGraphicsContext::ESolidPen);
    gc->SetPenColor(KRgbWhite);
    gc->SetPenSize(TSize(Max(1, sd / 10), Max(1, sd / 10)));
    gc->DrawLine(TPoint(x + sd * 38 / 100, y + sd * 62 / 100), TPoint(x + sd * 10 / 100, y + sd * 90 / 100));
    CleanupStack::PopAndDestroy(2);
    CGulIcon *icon = CGulIcon::NewL(bmp, mask);
    CleanupStack::Pop(2);
    return icon;
}

static CGulIcon *SavedIconL(const TSize &aSize)
{
    CFbsBitmap *bmp = new (ELeave) CFbsBitmap;
    CleanupStack::PushL(bmp);
    User::LeaveIfError(bmp->Create(aSize, EColor64K));
    CFbsBitmapDevice *dev = CFbsBitmapDevice::NewL(bmp);
    CleanupStack::PushL(dev);
    CFbsBitGc *gc = NULL;
    User::LeaveIfError(dev->CreateContext(gc));
    CleanupStack::PushL(gc);
    gc->SetPenStyle(CGraphicsContext::ENullPen);
    gc->SetBrushStyle(CGraphicsContext::ESolidBrush);
    gc->SetBrushColor(TRgb(0x6C, 0xB8, 0xF5));
    gc->DrawRect(TRect(aSize));
    TInt side = Min(aSize.iWidth, aSize.iHeight);
    TInt x0 = (aSize.iWidth - side) / 2, y0 = (aSize.iHeight - side) / 2;
    TInt l = x0 + side * 36 / 100, r = x0 + side * 64 / 100, cx = x0 + side / 2;
    TInt t = y0 + side * 29 / 100, b = y0 + side * 71 / 100, notch = y0 + side * 58 / 100;
    gc->SetPenStyle(CGraphicsContext::ESolidPen);
    gc->SetPenColor(KRgbWhite);
    gc->SetPenSize(TSize(Max(2, side / 16), Max(2, side / 16)));
    gc->SetBrushStyle(CGraphicsContext::ENullBrush);
    TPoint pts[5] = {TPoint(l, t), TPoint(r, t), TPoint(r, b), TPoint(cx, notch), TPoint(l, b)};
    gc->DrawPolygon(pts, 5);
    CleanupStack::PopAndDestroy(2);
    CFbsBitmap *mask = RoundMaskLC(aSize);
    CGulIcon *icon = CGulIcon::NewL(bmp, mask);
    CleanupStack::Pop(2);
    return icon;
}

static CFbsBitmap *RoundedSquareMaskLC(const TSize &aSize, TInt aRadius)
{
    CFbsBitmap *mask = new (ELeave) CFbsBitmap;
    CleanupStack::PushL(mask);
    User::LeaveIfError(mask->Create(aSize, EGray256));
    HBufC8 *line = HBufC8::NewLC(aSize.iWidth + 4);
    TPtr8 lp = line->Des();
    TInt side = Min(aSize.iWidth, aSize.iHeight);
    TReal x0 = (aSize.iWidth - side) / 2.0, y0 = (aSize.iHeight - side) / 2.0, r = aRadius;
    for (TInt y = 0; y < aSize.iHeight; y++) {
        lp.Zero();
        for (TInt x = 0; x < aSize.iWidth; x++) {
            TReal px = x + 0.5, py = y + 0.5;
            if (px < x0 || px > x0 + side || py < y0 || py > y0 + side) {
                lp.Append(0);
                continue;
            }
            TReal cx = Max(x0 + r, Min(px, x0 + side - r)), cy = Max(y0 + r, Min(py, y0 + side - r));
            TReal d;
            Math::Sqrt(d, (px - cx) * (px - cx) + (py - cy) * (py - cy));
            TReal a = r - d + 0.5;
            lp.Append(a <= 0 ? 0 : a >= 1 ? 255 : TUint8(a * 255));
        }
        mask->SetScanLine(lp, y);
    }
    CleanupStack::PopAndDestroy(line);
    return mask;
}

static CGulIcon *LetterIconL(const TSize &aSize, const TDesC &aName, TInt64 aId)
{
    static const TUint32 colors[] = {0xE17076, 0xFAA774, 0xA695E7, 0x7BC862,
                                     0x6EC9CB, 0x65AADD, 0xEE7AAE};
    TUint32 c = colors[(TUint)(I64LOW(aId) % 7)];
    CFbsBitmap *bmp = new (ELeave) CFbsBitmap;
    CleanupStack::PushL(bmp);
    User::LeaveIfError(bmp->Create(aSize, EColor64K));
    CFbsBitmapDevice *dev = CFbsBitmapDevice::NewL(bmp);
    CleanupStack::PushL(dev);
    CFbsBitGc *gc = NULL;
    User::LeaveIfError(dev->CreateContext(gc));
    CleanupStack::PushL(gc);
    gc->SetPenStyle(CGraphicsContext::ENullPen);
    gc->SetBrushStyle(CGraphicsContext::ESolidBrush);
    gc->SetBrushColor(TRgb(c >> 16, (c >> 8) & 255, c & 255));
    gc->DrawRect(TRect(aSize));
    TBuf<2> letter;
    for (TInt i = 0; i < aName.Length() && !letter.Length(); i++)
        if (TChar(aName[i]).IsAlphaDigit())
            letter.Append(TChar(aName[i]).GetUpperCase());
    if (!letter.Length())
        letter.Append('?');
    TFontSpec spec = CEikonEnv::Static()->TitleFont()->FontSpecInTwips();
    spec.iHeight = aSize.iHeight * 6 / 10;
    spec.iFontStyle.SetStrokeWeight(EStrokeWeightBold);
    CFont *font = NULL;
    if (dev->GetNearestFontToDesignHeightInPixels(font, spec) == KErrNone) {
        gc->UseFont(font);
        gc->SetPenStyle(CGraphicsContext::ESolidPen);
        gc->SetPenColor(KRgbWhite);
        gc->SetBrushStyle(CGraphicsContext::ENullBrush);
        TInt base = CapsBaselineL(font, aSize);
        gc->DrawText(letter, TRect(aSize), base, CGraphicsContext::ECenter);
        gc->DiscardFont();
        dev->ReleaseFont(font);
    }
    CleanupStack::PopAndDestroy(2);
    CFbsBitmap *mask = RoundMaskLC(aSize);
    CGulIcon *icon = CGulIcon::NewL(bmp, mask);
    CleanupStack::Pop(2);
    return icon;
}

class CTgRowView : public CFormattedCellListBoxView
{
public:
    ~CTgRowView() { iRight.Close(); }
    void DrawItem(TInt aIndex) const
    {
        CFormattedCellListBoxData *d =
            static_cast<CFormattedCellListBoxItemDrawer *>(ItemDrawer())->FormattedCellData();
        TBool bare = aIndex >= 0 && aIndex < iModel->NumberOfItems() &&
                     static_cast<MTextListBoxModel *>(iModel)->ItemText(aIndex).Locate('\t') == 0;
        TBool right = iRight.FindInOrder(aIndex) >= 0;
        TPoint p4 = d->SubCellPosition(4);
        TSize s4 = d->SubCellSize(4), s1w = d->SubCellSize(1);
        TInt extra = 0;
        if (aIndex >= 0 && aIndex < iModel->NumberOfItems() && d->IconArray()) {
            TPtrC t = static_cast<MTextListBoxModel *>(iModel)->ItemText(aIndex);
            for (TInt tabs = 0; tabs < 4; tabs++) {
                TInt at = t.Locate('\t');
                t.Set(at < 0 ? TPtrC() : t.Mid(at + 1));
            }
            TLex lex(t);
            TInt idx;
            if (t.Length() && lex.Val(idx) == KErrNone && idx >= 0 && idx < d->IconArray()->Count())
                extra = d->IconArray()->At(idx)->Bitmap()->SizeInPixels().iWidth - s4.iWidth;
        }
        if (extra > 0) {
            TRAP_IGNORE(d->SetSubCellPositionL(4, p4 - TPoint(extra, 0));
                        d->SetSubCellSizeL(4, s4 + TSize(extra, 0)));
        }
        TPoint p0 = d->SubCellPosition(0), p1 = d->SubCellPosition(1), p2 = d->SubCellPosition(2);
        TSize s1 = d->SubCellSize(1), s2 = d->SubCellSize(2);
        CGraphicsContext::TTextAlign a1 = d->SubCellAlignment(1), a2 = d->SubCellAlignment(2);
        TInt dx = Min(p1.iX, p2.iX) - p0.iX;
        TBool moved = dx > 0 && (bare || right);
        if (moved) {
            TRAP_IGNORE(
                d->SetSubCellPositionL(1, p1 - TPoint(dx, 0));
                d->SetSubCellPositionL(2, p2 - TPoint(dx, 0));
                if (bare) {
                    d->SetSubCellSizeL(1, s1 + TSize(dx, 0));
                    d->SetSubCellSizeL(2, s2 + TSize(dx, 0));
                } else {
                    d->SetSubCellPositionL(0, TPoint(Max(p1.iX + s1.iWidth, p2.iX + s2.iWidth) -
                                                         d->SubCellSize(0).iWidth, p0.iY));
                }
                if (right) {
                    d->SetSubCellAlignmentL(1, CGraphicsContext::ERight);
                    d->SetSubCellAlignmentL(2, CGraphicsContext::ERight);
                });
        }
        if (extra > 0) {
            TSize cur = d->SubCellSize(1);
            TRAP_IGNORE(d->SetSubCellSizeL(1, cur - TSize(Min(extra, cur.iWidth / 2), 0)));
        }
        CFormattedCellListBoxView::DrawItem(aIndex);
        if (extra > 0) {
            TRAP_IGNORE(d->SetSubCellPositionL(4, p4); d->SetSubCellSizeL(4, s4));
            if (!moved)
                TRAP_IGNORE(d->SetSubCellSizeL(1, s1w));
        }
        if (moved) {
            TRAP_IGNORE(d->SetSubCellPositionL(0, p0); d->SetSubCellPositionL(1, p1);
                        d->SetSubCellPositionL(2, p2); d->SetSubCellSizeL(1, s1);
                        d->SetSubCellSizeL(2, s2); d->SetSubCellAlignmentL(1, a1);
                        d->SetSubCellAlignmentL(2, a2));
        }
    }
    RArray<TInt> iRight;
};

class CTgLargeList : public CAknDoubleLargeStyleListBox
{
protected:
    CListBoxView *MakeViewClassInstanceL() { return new (ELeave) CTgRowView; }
};

class CTgList : public CCoeControl, public MEikListBoxObserver
{
public:
    static CTgList *NewL(const TRect &aRect, CTgAppUi &aUi, TBool aIcons);
    ~CTgList()
    {
        delete iFind;
        delete iListBox;
        delete iItems;
    }
    TBool HasIcons() const { return iIcons; }
    void ResetL()
    {
        if (iRowView)
            iRowView->iRight.Reset();
        iItems->Reset();
        Icons()->ResetAndDestroy();
        iListBox->HandleItemRemovalL();
    }
    void SetEmptyTextL(const TDesC &aText) { iListBox->View()->SetListEmptyTextL(aText); }
    TSize IconSize() const
    {
        TSize s = iListBox->ItemDrawer()->FormattedCellData()->SubCellSize(0);
        return s.iWidth > 8 && s.iHeight > 8 ? s : TSize(40, 40);
    }
    TInt AddIconL(CGulIcon *aIcon)
    {
        CleanupStack::PushL(aIcon);
        Icons()->AppendL(aIcon);
        CleanupStack::Pop(aIcon);
        return Icons()->Count() - 1;
    }
    TSize SubIconSize(TInt aSubCell) const
    {
        TSize s = iListBox->ItemDrawer()->FormattedCellData()->SubCellSize(aSubCell);
        return s.iWidth > 4 && s.iHeight > 4 ? s : TSize(16, 16);
    }
    void AddRowL(TInt aIcon, const TDesC &aTitle, const TDesC &aSub, TInt aInd1 = -1,
                 TInt aInd2 = -1, TBool aRight = EFalse)
    {
        if (aRight && iRowView)
            iRowView->iRight.AppendL(iItems->Count());
        HBufC *row = HBufC::NewLC(aTitle.Length() + aSub.Length() + 24);
        TPtr p = row->Des();
        if (aIcon >= 0)
            p.AppendNum(aIcon);
        p.Append('\t');
        TInt start = p.Length();
        p.Append(aTitle);
        p.Append('\t');
        TInt tab = p.Length() - 1;
        p.Append(aSub);
        for (TInt i = start; i < p.Length(); i++)
            if (i != tab && p[i] < 32)
                p[i] = ' ';
        if (aInd1 >= 0 || aInd2 >= 0) {
            p.Append('\t');
            if (aInd1 >= 0)
                p.AppendNum(aInd1);
            p.Append('\t');
            if (aInd2 >= 0)
                p.AppendNum(aInd2);
        }
        iItems->AppendL(*row);
        CleanupStack::PopAndDestroy(row);
    }
    CAknListBoxFilterItems *Filter() const
    {
        return iFind ? static_cast<CAknFilteredTextListBoxModel *>(iListBox->Model())->Filter()
                     : NULL;
    }
    void DoneL(TInt aCurrent)
    {
        iListBox->HandleItemAdditionL();
        if (aCurrent >= iItems->Count())
            aCurrent = iItems->Count() - 1;
        if (CAknListBoxFilterItems *f = Filter()) {
            f->HandleItemArrayChangeL();
            if (aCurrent >= 0)
                aCurrent = f->VisibleItemIndex(aCurrent);
            if (aCurrent < 0 && f->FilteredNumberOfItems() > 0)
                aCurrent = 0;
        }
        if (aCurrent >= 0)
            iListBox->SetCurrentItemIndex(aCurrent);
        iListBox->DrawDeferred();
    }
    TBool Searching() const { return iFind && iFind->IsVisible(); }
    void SetIconL(TInt aIndex, CGulIcon *aIcon)
    {
        CArrayPtr<CGulIcon> *icons = Icons();
        if (aIndex < 0 || aIndex >= icons->Count()) {
            delete aIcon;
            return;
        }
        delete icons->At(aIndex);
        icons->At(aIndex) = aIcon;
        iListBox->DrawDeferred();
    }
    void SetCurrentL(TInt aIndex)
    {
        if (aIndex >= 0 && aIndex < iItems->Count())
            iListBox->SetCurrentItemIndexAndDraw(aIndex);
    }
    void SetTopL(TInt aTop)
    {
        if (aTop >= 0 && aTop < iItems->Count()) {
            iListBox->SetTopItemIndex(aTop);
            iListBox->DrawDeferred();
        }
    }
    TInt CurrentIndex() const
    {
        TInt i = iListBox->CurrentItemIndex();
        if (CAknListBoxFilterItems *f = Filter())
            i = i >= 0 ? f->FilteredItemIndex(i) : -1;
        return i;
    }
    TInt Count() const { return iItems->Count(); }
    TKeyResponse OfferKeyEventL(const TKeyEvent &aKey, TEventCode aType);
    void HandleListBoxEventL(CEikListBox *aListBox, TListBoxEvent aEvent);

private:
    CTgList(CTgAppUi &aUi, TBool aIcons) : iUi(aUi), iIcons(aIcons) {}
    CArrayPtr<CGulIcon> *Icons() const
    {
        return iListBox->ItemDrawer()->FormattedCellData()->IconArray();
    }
    void ConstructL(const TRect &aRect)
    {
        CreateWindowL();
        if (iIcons)
            iListBox = new (ELeave) CTgLargeList;
        else
            iListBox = new (ELeave) CAknDoubleStyleListBox;
        iListBox->ConstructL(this, EAknListBoxSelectionList);
        if (iIcons)
            iRowView = static_cast<CTgRowView *>(iListBox->View());
        iListBox->SetListBoxObserver(this);
        iItems = new (ELeave) CDesCArrayFlat(32);
        iListBox->Model()->SetItemTextArray(iItems);
        iListBox->Model()->SetOwnershipType(ELbmDoesNotOwnItemArray);
        CArrayPtr<CGulIcon> *icons = new (ELeave) CArrayPtrFlat<CGulIcon>(32);
        iListBox->ItemDrawer()->FormattedCellData()->SetIconArray(icons);
        iListBox->View()->SetListEmptyTextL(KNullDesC);
        SetRect(aRect);
        ActivateL();
    }
    void SizeChanged()
    {
        iListBox->SetRect(Rect());
        if (iFind)
            AknFind::HandlePopupFindSizeChanged(this, iListBox, iFind);
    }
    TInt CountComponentControls() const { return iFind ? 2 : 1; }
    CCoeControl *ComponentControl(TInt aIndex) const
    {
        return aIndex ? static_cast<CCoeControl *>(iFind) : iListBox;
    }

    CTgAppUi &iUi;
    TBool iIcons;
    CEikFormattedCellListBox *iListBox;
    CTgRowView *iRowView;
    CDesCArrayFlat *iItems;
    CAknSearchField *iFind;
};

class CTgImage : public CCoeControl
{
public:
    static CTgImage *NewL(const TRect &aRect, CTgAppUi &aUi, CFbsBitmap *aBitmap);
    ~CTgImage() { delete iBitmap; }
    TKeyResponse OfferKeyEventL(const TKeyEvent &aKey, TEventCode aType);
    void SetBitmap(CFbsBitmap *aBitmap)
    {
        delete iBitmap;
        iBitmap = aBitmap;
        DrawDeferred();
    }
    TInt Zoom() const { return iZoom; }
    void SetZoom(TInt aZoom)
    {
        iZoom = aZoom < 1 ? 1 : aZoom > 4 ? 4 : aZoom;
        if (iZoom == 1)
            iPanX = iPanY = 0.5;
        DrawDeferred();
    }
    TBool Pan(TInt aDx, TInt aDy)
    {
        if (iZoom == 1)
            return EFalse;
        TReal step = 0.25 / iZoom;
        iPanX = Clamp(iPanX + aDx * step);
        iPanY = Clamp(iPanY + aDy * step);
        DrawDeferred();
        return ETrue;
    }

private:
    CTgImage(CTgAppUi &aUi, CFbsBitmap *aBitmap)
        : iUi(aUi), iBitmap(aBitmap), iZoom(1), iPanX(0.5), iPanY(0.5) {}
    static TReal Clamp(TReal v) { return v < 0 ? 0 : v > 1 ? 1 : v; }
    static void Axis(TInt aView, TInt aBmp, TReal aScale, TReal aPan, TInt &aSrc, TInt &aSrcLen,
                     TInt &aDst, TInt &aDstLen)
    {
        TReal vis = aView / aScale;
        if (vis >= aBmp) {
            aSrc = 0;
            aSrcLen = aBmp;
            aDstLen = TInt(aBmp * aScale);
            aDst = (aView - aDstLen) / 2;
        } else {
            TReal start = aPan * aBmp - vis / 2;
            if (start < 0)
                start = 0;
            if (start > aBmp - vis)
                start = aBmp - vis;
            aSrc = TInt(start);
            aSrcLen = Max(1, TInt(vis));
            aDst = 0;
            aDstLen = aView;
        }
    }
    void Draw(const TRect &) const
    {
        CWindowGc &gc = SystemGc();
        TRect r = Rect();
        gc.SetPenStyle(CGraphicsContext::ENullPen);
        gc.SetBrushStyle(CGraphicsContext::ESolidBrush);
        gc.SetBrushColor(KRgbBlack);
        gc.DrawRect(r);
        TSize b = iBitmap->SizeInPixels();
        if (b.iWidth <= 0 || b.iHeight <= 0)
            return;
        TReal fit = Min(TReal(r.Width()) / b.iWidth, TReal(r.Height()) / b.iHeight);
        TReal scale = fit * iZoom;
        TInt sx, sw, dx, dw, sy, sh, dy, dh;
        Axis(r.Width(), b.iWidth, scale, iPanX, sx, sw, dx, dw);
        Axis(r.Height(), b.iHeight, scale, iPanY, sy, sh, dy, dh);
        gc.DrawBitmap(TRect(TPoint(r.iTl.iX + dx, r.iTl.iY + dy), TSize(dw, dh)), iBitmap,
                      TRect(TPoint(sx, sy), TSize(sw, sh)));
    }

    CTgAppUi &iUi;
    CFbsBitmap *iBitmap;
    TInt iZoom;
    TReal iPanX;
    TReal iPanY;
};

class CTgCamera : public CCoeControl, public MCameraObserver
{
public:
    static CTgCamera *NewL(const TRect &aRect, CTgAppUi &aUi);
    ~CTgCamera()
    {
        if (iCamera) {
            iCamera->StopViewFinder();
            iCamera->PowerOff();
            iCamera->Release();
            delete iCamera;
        }
        delete iFrame;
    }
    void Capture()
    {
        if (iReady && !iCapturing) {
            iCapturing = ETrue;
            iCamera->CaptureImage();
        }
    }
    TKeyResponse OfferKeyEventL(const TKeyEvent &aKey, TEventCode aType);

private:
    CTgCamera(CTgAppUi &aUi) : iUi(aUi) {}
    void ConstructL(const TRect &aRect)
    {
        CreateWindowL();
        SetRect(aRect);
        ActivateL();
        if (CCamera::CamerasAvailable() < 1)
            User::Leave(KErrNotSupported);
        iCamera = CCamera::NewL(*this, 0);
        iCamera->Reserve();
    }
    void ReserveComplete(TInt aError);
    void PowerOnComplete(TInt aError);
    void ViewFinderFrameReady(CFbsBitmap &aFrame)
    {
        if (!iFrame)
            iFrame = new CFbsBitmap;
        if (iFrame && iFrame->Duplicate(aFrame.Handle()) == KErrNone)
            DrawDeferred();
    }
    void ImageReady(CFbsBitmap *aBitmap, HBufC8 *aData, TInt aError);
    void FrameBufferReady(MFrameBuffer *, TInt) {}
    void Draw(const TRect &) const
    {
        CWindowGc &gc = SystemGc();
        TRect r = Rect();
        gc.SetPenStyle(CGraphicsContext::ENullPen);
        gc.SetBrushStyle(CGraphicsContext::ESolidBrush);
        gc.SetBrushColor(KRgbBlack);
        gc.DrawRect(r);
        if (!iFrame || !iFrame->Handle())
            return;
        TSize b = iFrame->SizeInPixels();
        if (b.iWidth <= 0 || b.iHeight <= 0)
            return;
        TInt w = r.Width(), h = b.iHeight * r.Width() / b.iWidth;
        if (h > r.Height()) {
            h = r.Height();
            w = b.iWidth * r.Height() / b.iHeight;
        }
        gc.DrawBitmap(TRect(TPoint(r.iTl.iX + (r.Width() - w) / 2, r.iTl.iY + (r.Height() - h) / 2),
                            TSize(w, h)), iFrame);
    }

    CTgAppUi &iUi;
    CCamera *iCamera;
    CFbsBitmap *iFrame;
    TBool iReady;
    TBool iCapturing;
};

struct TTgChip
    {
    CGulIcon *iIcon;
    TBuf<12> iText;
    TInt iCount;
    };

class CTgMsgView : public CCoeControl
{
public:
    static CTgMsgView *NewL(const TRect &aClient, CTgAppUi &aUi, const TDesC &aHead, HBufC *aBody,
                            RArray<TTgChip> &aChips);
    ~CTgMsgView()
    {
        for (TInt i = 0; i < iChips.Count(); i++)
            delete iChips[i].iIcon;
        iChips.Close();
        delete iLines;
        delete iHead;
        delete iBody;
    }
    TKeyResponse OfferKeyEventL(const TKeyEvent &aKey, TEventCode aType);
    static TInt IconSide()
    {
        return AknLayoutUtils::FontFromId(EAknLogicalFontSecondaryFont)->HeightInPixels() + 2;
    }

private:
    CTgMsgView(CTgAppUi &aUi) : iUi(aUi) {}
    TInt LineH() const { return iBodyFont->HeightInPixels() + 3; }
    TInt ChipW(const TTgChip &c) const
    {
        TBuf<24> t;
        if (!c.iIcon)
            t.Append(c.iText);
        t.AppendNum(c.iCount);
        return (c.iIcon ? IconSide() + 3 : 0) + iBodyFont->TextWidthInPixels(t) + 10;
    }
    TInt ChipRows(TInt aWidth) const
    {
        TInt rows = iChips.Count() ? 1 : 0, x = 0;
        for (TInt i = 0; i < iChips.Count(); i++) {
            TInt w = ChipW(iChips[i]);
            if (x && x + w > aWidth) {
                rows++;
                x = 0;
            }
            x += w;
        }
        return rows;
    }
    void ConstructL(const TRect &aClient, const TDesC &aHead, HBufC *aBody, RArray<TTgChip> &aChips)
    {
        iBody = aBody;
        iHead = aHead.AllocL();
        for (TInt i = 0; i < aChips.Count(); i++) {
            iChips.AppendL(aChips[i]);
            aChips[i].iIcon = NULL;
        }
        iHeadFont = AknLayoutUtils::FontFromId(EAknLogicalFontPrimaryFont);
        iBodyFont = AknLayoutUtils::FontFromId(EAknLogicalFontSecondaryFont);
        iTextW = aClient.Width() - 24;
        iLines = new (ELeave) CArrayFixFlat<TPtrC>(16);
        TPtrC rest(*iBody);
        while (rest.Length() || !iLines->Count()) {
            TInt nl = rest.Locate('\n');
            TPtrC para(nl >= 0 ? rest.Left(nl) : rest);
            if (!para.Length())
                iLines->AppendL(TPtrC());
            while (para.Length()) {
                TInt n = iBodyFont->TextCount(para, iTextW);
                if (n <= 0)
                    n = 1;
                if (n < para.Length()) {
                    TInt sp = para.Left(n).LocateReverse(' ');
                    if (sp > 0)
                        n = sp + 1;
                }
                TPtrC line = para.Left(n);
                while (line.Length() && line[line.Length() - 1] == ' ')
                    line.Set(line.Left(line.Length() - 1));
                iLines->AppendL(line);
                para.Set(para.Mid(n));
            }
            if (nl < 0)
                break;
            rest.Set(rest.Mid(nl + 1));
        }
        iHeadH = iHeadFont->HeightInPixels() + 10;
        CreateWindowL();
        SetRect(aClient);
        ActivateL();
    }
    void Draw(const TRect &) const
    {
        CWindowGc &gc = SystemGc();
        TRect r = Rect();
        MAknsSkinInstance *skin = AknsUtils::SkinInstance();
        TRect inner = r;
        inner.Shrink(7, 7);
        if (!AknsDrawUtils::DrawFrame(skin, gc, r, inner, KAknsIIDQsnFrPopup, KAknsIIDQsnFrPopupCenter)) {
            gc.SetPenStyle(CGraphicsContext::ESolidPen);
            gc.SetPenColor(KRgbGray);
            gc.SetBrushStyle(CGraphicsContext::ESolidBrush);
            gc.SetBrushColor(KRgbWhite);
            gc.DrawRect(r);
        }
        TRgb color = KRgbBlack;
        AknsUtils::GetCachedColor(skin, color, KAknsIIDQsnTextColors, EAknsCIQsnTextColorsCG19);
        TRect head(TPoint(r.iTl.iX + 4, r.iTl.iY + 4), TSize(r.Width() - 8, iHeadH));
        TRect headInner = head;
        headInner.Shrink(4, 3);
        AknsDrawUtils::DrawFrame(skin, gc, head, headInner, KAknsIIDQsnFrPopupHeading,
                                 KAknsIIDQsnFrPopupHeadingCenter);
        gc.SetBrushStyle(CGraphicsContext::ENullBrush);
        gc.SetPenStyle(CGraphicsContext::ESolidPen);
        gc.SetPenColor(color);
        gc.UseFont(iHeadFont);
        TInt base = (iHeadH - iHeadFont->HeightInPixels()) / 2 + iHeadFont->AscentInPixels();
        gc.DrawText(*iHead, TRect(head.iTl.iX + 8, head.iTl.iY, head.iBr.iX - 8, head.iBr.iY), base,
                    CGraphicsContext::ELeft);
        gc.DiscardFont();
        gc.UseFont(iBodyFont);
        TInt x0 = r.iTl.iX + 12, y = head.iBr.iY + 6, bottom = r.iBr.iY - 8, row = 0;
        TInt lh = LineH();
        for (TInt i = 0; i < iLines->Count(); i++, row++) {
            if (row < iTop)
                continue;
            if (y + lh > bottom)
                break;
            gc.DrawText(iLines->At(i), TPoint(x0, y + iBodyFont->AscentInPixels()));
            y += lh;
        }
        if (iChips.Count() && y + lh <= bottom) {
            TInt ch = Max(lh, IconSide() + 4), x = x0;
            if (row >= iTop)
                y += 6;
            TInt chipRow = row;
            for (TInt i = 0; i < iChips.Count(); i++) {
                const TTgChip &c = iChips[i];
                TInt w = ChipW(c);
                if (x > x0 && x - x0 + w > iTextW) {
                    x = x0;
                    if (chipRow >= iTop)
                        y += ch;
                    chipRow++;
                }
                if (chipRow >= iTop && y + ch <= bottom) {
                    TInt cx = x;
                    if (c.iIcon) {
                        TSize is = c.iIcon->Bitmap()->SizeInPixels();
                        gc.BitBltMasked(TPoint(cx, y + (ch - is.iHeight) / 2), c.iIcon->Bitmap(),
                                        TRect(is), c.iIcon->Mask(), EFalse);
                        cx += is.iWidth + 3;
                    }
                    TBuf<24> t;
                    if (!c.iIcon)
                        t.Append(c.iText);
                    t.AppendNum(c.iCount);
                    gc.DrawText(t, TPoint(cx, y + (ch - iBodyFont->HeightInPixels()) / 2 +
                                                  iBodyFont->AscentInPixels()));
                }
                x += w;
            }
        }
        gc.DiscardFont();
    }

    CTgAppUi &iUi;
    HBufC *iHead;
    HBufC *iBody;
    CArrayFixFlat<TPtrC> *iLines;
    RArray<TTgChip> iChips;
    const CFont *iHeadFont;
    const CFont *iBodyFont;
    TInt iHeadH;
    TInt iTextW;
    TInt iTop;
};

class MTgDecodeObserver
{
public:
    virtual void DecodedL(TInt64 aKey, TInt aKind, CFbsBitmap *aBitmap, CFbsBitmap *aMask) = 0;
};

class CTgDecoder : public CActive
{
public:
    CTgDecoder(MTgDecodeObserver &aObserver, RFs &aFs)
        : CActive(EPriorityLow), iObserver(aObserver), iFs(aFs)
    {
        CActiveScheduler::Add(this);
    }
    ~CTgDecoder()
    {
        Cancel();
        delete iDecoder;
        delete iBitmap;
        delete iMask;
        iQueue.Close();
    }
    void AddL(const TDesC &aPath, TInt64 aKey, TInt aMedia, const TSize &aTarget)
    {
        TReq r;
        r.iPath = aPath;
        r.iKey = aKey;
        r.iMedia = aMedia;
        r.iTarget = aTarget;
        if (aMedia)
            iQueue.InsertL(r, IsActive() ? 1 : 0);
        else
            iQueue.AppendL(r);
        if (!IsActive())
            Next();
    }

private:
    struct TReq
        {
        TFileName iPath;
        TInt64 iKey;
        TInt iMedia;
        TSize iTarget;
        };
    void Next()
    {
        while (iQueue.Count() && !IsActive()) {
            TRAPD(err, StartL());
            if (err != KErrNone) {
                TReq r = iQueue[0];
                iQueue.Remove(0);
                delete iDecoder;
                iDecoder = NULL;
                delete iBitmap;
                iBitmap = NULL;
                delete iMask;
                iMask = NULL;
                if (r.iMedia != 3)
                    iFs.Delete(r.iPath);
                TRAP_IGNORE(iObserver.DecodedL(r.iKey, r.iMedia, NULL, NULL));
            }
        }
    }
    void StartL()
    {
        const TReq &r = iQueue[0];
        iDecoder = CImageDecoder::FileNewL(iFs, r.iPath);
        TSize f = iDecoder->FrameInfo(0).iOverallSizeInPixels;
        TInt k = 0;
        while (k < 3 && (f.iWidth >> (k + 1)) >= r.iTarget.iWidth &&
               (f.iHeight >> (k + 1)) >= r.iTarget.iHeight)
            k++;
        TSize size((f.iWidth + (1 << k) - 1) >> k, (f.iHeight + (1 << k) - 1) >> k);
        iBitmap = new (ELeave) CFbsBitmap;
        User::LeaveIfError(iBitmap->Create(size, EColor64K));
        if (r.iMedia == 3) {
            iMask = new (ELeave) CFbsBitmap;
            User::LeaveIfError(iMask->Create(size, EGray256));
            iDecoder->Convert(&iStatus, *iBitmap, *iMask);
        } else {
            iDecoder->Convert(&iStatus, *iBitmap);
        }
        SetActive();
    }
    void RunL()
    {
        TReq r = iQueue[0];
        iQueue.Remove(0);
        delete iDecoder;
        iDecoder = NULL;
        CFbsBitmap *bmp = iBitmap, *mask = iMask;
        iBitmap = NULL;
        iMask = NULL;
        if (iStatus.Int() != KErrNone) {
            delete bmp;
            delete mask;
            bmp = mask = NULL;
            if (r.iMedia != 3)
                iFs.Delete(r.iPath);
        }
        TRAP_IGNORE(iObserver.DecodedL(r.iKey, r.iMedia, bmp, mask));
        Next();
    }
    void DoCancel()
    {
        if (iDecoder)
            iDecoder->Cancel();
    }

    MTgDecodeObserver &iObserver;
    RFs &iFs;
    RArray<TReq> iQueue;
    CImageDecoder *iDecoder;
    CFbsBitmap *iBitmap;
    CFbsBitmap *iMask;
};

struct TTgDraft
    {
    TInt64 iId;
    TInt iKind;
    HBufC *iText;
    };

struct TTgAvatar
    {
    TInt64 iId;
    CFbsBitmap *iBitmap;
    CFbsBitmap *iMask;
    };

class CTgQr : public CCoeControl
{
public:
    static CTgQr *NewL(const TRect &aRect)
    {
        CTgQr *self = new (ELeave) CTgQr;
        CleanupStack::PushL(self);
        self->CreateWindowL();
        self->SetRect(aRect);
        self->ActivateL();
        CleanupStack::Pop(self);
        return self;
    }
    void SetTextL(const char *aText)
    {
        iSize = tg_qr_encode(aText, iModules);
        DrawDeferred();
    }

private:
    void Draw(const TRect &) const
    {
        CWindowGc &gc = SystemGc();
        TRect r = Rect();
        gc.SetPenStyle(CGraphicsContext::ENullPen);
        gc.SetBrushStyle(CGraphicsContext::ESolidBrush);
        gc.SetBrushColor(KRgbWhite);
        gc.DrawRect(r);
        if (!iSize)
            return;
        TInt side = Min(r.Width(), r.Height());
        TInt px = Max(1, side / (iSize + 4));
        TPoint o(r.iTl.iX + (side - px * iSize) / 2, r.iTl.iY + (r.Height() - px * iSize) / 2);
        gc.SetBrushColor(KRgbBlack);
        for (TInt y = 0; y < iSize; y++)
            for (TInt x = 0; x < iSize; x++)
                if (iModules[y * iSize + x])
                    gc.DrawRect(TRect(TPoint(o.iX + x * px, o.iY + y * px), TSize(px, px)));
        const CFont *font = CEikonEnv::Static()->DenseFont();
        gc.UseFont(font);
        gc.SetPenStyle(CGraphicsContext::ESolidPen);
        gc.SetPenColor(KRgbBlack);
        gc.SetBrushStyle(CGraphicsContext::ENullBrush);
        const TText *lines[] = {
            _S("On your main phone"), _S("open Telegram:"), _S(""), _S("Settings >"),
            _S("Devices >"), _S("Link Desktop"), _S("Device"), _S(""), _S("and scan this code.")};
        TInt lh = font->HeightInPixels() + 3;
        TInt tx = r.iTl.iX + side + 2;
        TInt ty = r.iTl.iY + (r.Height() - lh * 9) / 2 + font->AscentInPixels();
        for (TInt i = 0; i < 9; i++)
            gc.DrawText(T(TPtrC(lines[i])), TPoint(tx, ty + i * lh));
        gc.DiscardFont();
    }

    TInt iSize;
    unsigned char iModules[TG_QR_MAX * TG_QR_MAX];
};

enum TScreen
    {
    EScreenQr,
    EScreenStatus,
    EScreenChats,
    EScreenThread,
    EScreenImage,
    EScreenCamera
    };

class CTgAppUi : public CAknAppUi, public MTgObserver, public MTgDecodeObserver
{
    friend class CTgCamera;
    friend class CTgMsgView;
    friend class CTgImage;

public:
    void ConstructL()
    {
        BaseConstructL(EAknEnableSkin);
        InitPrivateDirL();
        ApplyLangL();
        rsym_net_start();
        iReaper = new (ELeave) CAsyncCallBack(TCallBack(Reap, this), CActive::EPriorityStandard);
        iWorker = CTgWorker::NewL(*this, iPrivDir);
        iQrTimer = CPeriodic::NewL(CActive::EPriorityStandard);
        iPollTimer = CPeriodic::NewL(CActive::EPriorityLow);
        iProgressTimer = CPeriodic::NewL(CActive::EPriorityLow);
        iDecoder = new (ELeave) CTgDecoder(*this, iEikonEnv->FsSession());
        iAfterCapture = new (ELeave) CAsyncCallBack(TCallBack(AfterCapture, this),
                                                    CActive::EPriorityStandard);
        {
            TFileName exe = RProcess().FileName(), priv;
            iEikonEnv->FsSession().PrivatePath(priv);
            for (TUint i = 0; i < sizeof KEmoji / sizeof KEmoji[0]; i++) {
                TFileName path;
                path.Append(exe.Length() ? TChar(exe[0]) : TChar('C'));
                path.Append(':');
                path.Append(priv);
                path.AppendFormat(_L("emoji\\emoji_u%x.png"), KEmoji[i].iCp);
                if (!FileExists(path))
                    path[0] = 'C';
                iDecoder->AddL(path, KEmoji[i].iCp, 3, TSize(32, 32));
            }
        }
        iPollTimer->Start(10000000, 10000000, TCallBack(PollTick, this));
        iForeground = ETrue;
        iNavi = static_cast<CAknNavigationControlContainer *>(
            StatusPane()->ControlL(TUid::Uid(EEikStatusPaneUidNavi)));
        RebuildTabsL();
        ShowStatusL(T(_L("Connecting...")));
        StartDialogsL(EFalse, ETrue);
    }

    ~CTgAppUi()
    {
        HideNaviLabel();
        HideTabs();
        delete iTabDeco;
        delete iVibra;
        delete iDocHandler;
        delete iPollTimer;
        delete iProgressTimer;
        delete iQrTimer;
        delete iWorker;
        delete iDecoder;
        for (TInt i = 0; i < iAvatars.Count(); i++) {
            delete iAvatars[i].iBitmap;
            delete iAvatars[i].iMask;
        }
        iAvatars.Close();
        for (TInt i = 0; i < iEmoji.Count(); i++) {
            delete iEmoji[i].iBitmap;
            delete iEmoji[i].iMask;
        }
        iEmoji.Close();
        iAvatarAsked.Close();
        iWanted.Close();
        Retire(iList);
        Retire(iQr);
        Retire(iImage);
        Retire(iCameraCtl);
        if (iMsgView) {
            RemoveFromStack(iMsgView);
            delete iMsgView;
            iMsgView = NULL;
        }
        delete iAfterCapture;
        delete iCaptured;
        for (TInt i = 0; i < iDead.Count(); i++)
            delete iDead[i];
        iDead.Close();
        iRows.Close();
        iMsgRows.Close();
        iAlbum.Close();
        iRowIcon.Close();
        for (TInt i = 0; i < iDrafts.Count(); i++)
            delete iDrafts[i].iText;
        iDrafts.Close();
        delete iPinned;
        iTabCounts.Close();
        delete iReaper;
        tg_dialogs_free(iDialogs, iNumDialogs);
        tg_dialogs_free(iArchive, iNumArchive);
        tg_folders_free(iFolders, iNumFolders);
        tg_messages_free(iMessages, iNumMessages);
        rsym_net_stop();
        User::Exit(KErrNone);
    }

    void SelectL()
    {
        if (iScreen == EScreenChats)
            OpenChatL();
        else if (iScreen == EScreenThread)
            ShowMessageL(EFalse);
        else if (iScreen == EScreenCamera && iCameraCtl)
            iCameraCtl->Capture();
        else if (iScreen == EScreenImage && iImage)
            ZoomL(iImage->Zoom() >= 4 ? -4 : iImage->Zoom());
        else if (iScreen == EScreenStatus && iLoggedOut)
            StartQrLoginL();
    }

    TBool ShortcutL(TUint aCode)
    {
        if (iScreen == EScreenChats && (aCode == EKeyLeftArrow || aCode == EKeyRightArrow)) {
            SwitchTabL(aCode == EKeyLeftArrow ? -1 : 1);
            return ETrue;
        }
        if (iScreen == EScreenThread && aCode == EKeyDownArrow && iList && iList->Count() &&
            iList->CurrentIndex() == iList->Count() - 1)
            return ETrue;
        if (iScreen == EScreenThread && aCode == EKeyUpArrow && iList && iList->Count() &&
            iList->CurrentIndex() == 0) {
            LoadOlderL();
            return ETrue;
        }
        if ((iScreen == EScreenThread || iScreen == EScreenImage) && (aCode == 'r' || aCode == 'R')) {
            ReplyL();
            return ETrue;
        }
        if (iScreen == EScreenThread && aCode == EKeyBackspace) {
            tg_message *m = SelectedMessage();
            if (m && !(iCur.flags & TG_DLG_READONLY))
                DeleteMessageL(*m);
            return ETrue;
        }
        TUint k = aCode < 128 ? TUint(TChar(aCode).GetLowerCase()) : aCode;
        TBool readonly = (iCur.flags & TG_DLG_READONLY) != 0;
        TInt cmd = 0;
        if (iScreen == EScreenChats) {
            tg_dialog *d = NULL;
            CurrentChat(d);
            TInt f = d ? d->flags : 0;
            switch (k) {
            case 'm': cmd = d ? (f & TG_DLG_MUTED ? ETgCmdUnmute : ETgCmdMute) : 0; break;
            case 'p': cmd = d ? (f & TG_DLG_PINNED ? ETgCmdUnpin : ETgCmdPin) : 0; break;
            case 'a': cmd = d && !(f & TG_DLG_SELF) ? (f & TG_DLG_ARCHIVED ? ETgCmdUnarchive : ETgCmdArchive) : 0; break;
            case 'u': cmd = d ? (d->unread || (f & TG_DLG_UNREAD_MARK) ? ETgCmdMarkRead : ETgCmdMarkUnread) : 0; break;
            case 'i': cmd = d ? ETgCmdInfo : 0; break;
            case 'c': cmd = ETgCmdContacts; break;
            case 'f': cmd = ETgCmdFindUser; break;
            }
        } else if (iScreen == EScreenThread || iScreen == EScreenImage) {
            TBool thread = iScreen == EScreenThread;
            switch (k) {
            case 'q': cmd = thread && !readonly ? ETgCmdReplyTo : 0; break;
            case 'f': cmd = thread ? ETgCmdForward : 0; break;
            case 'l': cmd = thread ? ETgCmdReact : 0; break;
            case 'c': cmd = thread ? ETgCmdCopy : 0; break;
            case 'o': cmd = thread ? ETgCmdOpenLink : 0; break;
            case 's': cmd = ETgCmdSave; break;
            case 'p': cmd = iPinned ? ETgCmdPinned : 0; break;
            case 'i': cmd = ETgCmdInfo; break;
            case 'g': cmd = thread && !readonly ? ETgCmdSendPhoto : 0; break;
            case 'k': cmd = thread && !readonly ? ETgCmdCamera : 0; break;
            case 't':
            case 'b':
                if (thread && iList && iList->Count())
                    iList->SetCurrentL(k == 't' ? 0 : iList->Count() - 1);
                return thread;
            }
        }
        if (cmd) {
            HandleCommandL(cmd);
            return ETrue;
        }
        if (iScreen == EScreenThread && (aCode == 'e' || aCode == 'E')) {
            tg_message *m = SelectedMessage();
            if (m && CanEdit(*m))
                MessageActionL(ETgCmdEdit);
            return ETrue;
        }
        return EFalse;
    }

    void JobDoneL(TTgJob &aJob)
    {
        iInJobDone = ETrue;
        TRAPD(err, HandleJobL(aJob));
        iInJobDone = EFalse;
        iWorker->ClearOutputs();
        if (iHasPending && !iWorker->Busy()) {
            iHasPending = EFalse;
            StartJobL(iPending);
        }
        if (iChatsDirty && !iWorker->Busy() && iScreen == EScreenChats)
            TRAP_IGNORE(StartDialogsL(ETrue, EFalse));
        TRAP_IGNORE(KickDownloadsL());
        User::LeaveIfError(err);
    }

    void DecodedL(TInt64 aKey, TInt aMedia, CFbsBitmap *aBitmap, CFbsBitmap *aMask)
    {
        if (aMedia == 3) {
            if (aBitmap && aMask) {
                TTgAvatar e;
                e.iId = aKey;
                e.iBitmap = aBitmap;
                e.iMask = aMask;
                if (iEmoji.Append(e) == KErrNone) {
                    TBool used = EFalse;
                    for (TInt i = 0; i < iNumMessages && !used; i++)
                        for (TInt r = 0; r < iMessages[i].react_kinds && !used; r++)
                            used = iMessages[i].react_cp[r] == aKey;
                    if (used && iScreen == EScreenThread && iList)
                        ShowThreadL();
                    return;
                }
            }
            delete aBitmap;
            delete aMask;
            return;
        }
        delete aMask;
        if (aMedia == 2) {
            if (aBitmap && iScreen == EScreenImage && aKey == iViewKey && iImage)
                iImage->SetBitmap(aBitmap);
            else
                delete aBitmap;
            SetTitleL(iTitle);
            return;
        }
        if (aMedia == 1) {
            if (aBitmap && (iScreen == EScreenThread || iScreen == EScreenImage) && aKey == iViewKey)
                ShowImageL(aBitmap);
            else
                delete aBitmap;
            if (!aBitmap && aKey == iViewKey && iScreen == EScreenThread)
                InfoL(T(_L("Could not open this picture")));
            SetTitleL(iTitle);
            return;
        }
        if (!aBitmap)
            return;
        TBool thumb = aMedia == 4;
        CleanupStack::PushL(aBitmap);
        TSize size = thumb ? iThumbSize : iIconSize;
        TTgAvatar a;
        a.iId = aKey;
        a.iBitmap = new (ELeave) CFbsBitmap;
        CleanupStack::PushL(a.iBitmap);
        User::LeaveIfError(a.iBitmap->Create(size, EColor64K));
        CFbsBitmapDevice *dev = CFbsBitmapDevice::NewL(a.iBitmap);
        CleanupStack::PushL(dev);
        CFbsBitGc *gc = NULL;
        User::LeaveIfError(dev->CreateContext(gc));
        TInt side = Min(size.iWidth, size.iHeight);
        TRect dst(TPoint((size.iWidth - side) / 2, (size.iHeight - side) / 2), TSize(side, side));
        TSize bs = aBitmap->SizeInPixels();
        TInt bside = Min(bs.iWidth, bs.iHeight);
        TRect src(TPoint((bs.iWidth - bside) / 2, (bs.iHeight - bside) / 2), TSize(bside, bside));
        gc->DrawBitmap(dst, aBitmap, src);
        delete gc;
        CleanupStack::PopAndDestroy(dev);
        a.iMask = thumb ? RoundedSquareMaskLC(size, side / 5) : RoundMaskLC(size);
        iAvatars.AppendL(a);
        CleanupStack::Pop(2);
        CleanupStack::PopAndDestroy(aBitmap);
        if (thumb) {
            if (iScreen == EScreenThread && iList && !iMsgView)
                ShowThreadL();
            return;
        }
        if (iScreen == EScreenChats && iList)
            for (TInt i = 0; i < iRows.Count() && i < iList->Count(); i++)
                if (DialogAt(iRows[i])->photo.id == aKey)
                    iList->SetIconL(iRowIcon[i], AvatarIconL(iAvatars.Count() - 1));
    }

private:
    void HandleJobL(TTgJob &aJob)
    {
        TBool quiet = iQuiet;
        iQuiet = EFalse;
        SetTitleL(iTitle);
        TInt rc = aJob.iResult;
        switch (aJob.iType) {
        case EJobPoll:
            if (rc == TG_NEED_LOGIN)
                ShowLoggedOutL();
            else if (!rc && aJob.iChanged)
                RefreshL();
            break;
        case EJobAvatars:
            for (TInt i = 0; i < aJob.iNumLocs; i++)
                if (aJob.iLocOk[i]) {
                    TFileName path;
                    char cpath[128];
                    TBool avatar = aJob.iLocs[i].type == TG_LOC_PEER_PHOTO;
                    CachePath(avatar ? 'a' : 't', aJob.iLocs[i].id, avatar ? "" : aJob.iLocs[i].thumb,
                              path, cpath, sizeof cpath);
                    iDecoder->AddL(path, aJob.iLocs[i].id, avatar ? 0 : 4,
                                   avatar ? iIconSize : iThumbSize);
                }
            break;
        case EJobSendFile:
            iProgressTimer->Cancel();
            iChatsDirty = ETrue;
            if (rc)
                ShowErrorL(aJob.iError);
            else if (iScreen == EScreenThread)
                LoadThreadL(EFalse);
            break;
        case EJobMedia:
            SetTitleL(iTitle);
            if (aJob.iMediaPurpose == 2) {
                if (rc) {
                    ShowErrorL(aJob.iError);
                } else if (iOpenAfterSave) {
                    OpenExternalL(iSaveName);
                } else {
                    HBufC *msg = HBufC::NewLC(iSaveName.Length() + 16);
                    msg->Des().Copy(T(_L("Saved to ")));
                    msg->Des().Append(iSaveName);
                    InfoL(*msg);
                    CleanupStack::PopAndDestroy(msg);
                }
            } else if (rc) {
                if (iScreen == EScreenThread)
                    ShowErrorL(aJob.iError);
            } else {
                TFileName path;
                char cpath[128];
                TInt full = aJob.iMediaPurpose == 1;
                CachePath(full ? 'f' : 'm', aJob.iMediaLoc.id, aJob.iMediaLoc.thumb, path, cpath,
                          sizeof cpath);
                TSize target = ClientRect().Size();
                if (full)
                    target = TSize(target.iWidth * 4, target.iHeight * 4);
                iDecoder->AddL(path, aJob.iMediaLoc.id, full ? 2 : 1, target);
            }
            break;
        case EJobDialogs:
            if (rc == TG_NEED_LOGIN) {
                StartQrLoginL();
                break;
            }
            if (rc) {
                if (!quiet)
                    ShowErrorL(aJob.iError);
                if (iScreen == EScreenStatus && !iLoggedOut)
                    ShowStatusL(T(_L("Could not load chats\nOptions > Refresh")));
                break;
            }
            {
                tg_dialog *sel;
                if (CurrentChat(sel)) {
                    iSelId = sel->id;
                    iSelKind = sel->kind;
                }
            }
            if (aJob.iFolders) {
                TBool archive = iTab == ArchiveTab(), first = !iFoldersLoaded;
                tg_folders_free(iFolders, iNumFolders);
                iFolders = aJob.iFolders;
                iNumFolders = aJob.iNumFolders;
                aJob.iFolders = NULL;
                aJob.iNumFolders = 0;
                iFoldersLoaded = ETrue;
                if (archive)
                    iTab = ArchiveTab();
                else if (first || iTab >= ArchiveTab())
                    iTab = AllTab();
                RebuildTabsL();
            }
            NotifyL(aJob);
            iRows.Reset();
            if (aJob.iDoneFolder == 0 || (aJob.iDoneFolder == 2 && iTab != ArchiveTab()) ||
                (aJob.iDoneFolder == 1 && iTab == ArchiveTab()))
                iChatsDirty = EFalse;
            if (aJob.iDoneFolder != 1) {
                tg_dialogs_free(iDialogs, iNumDialogs);
                iDialogs = aJob.iDialogs;
                iNumDialogs = aJob.iNumDialogs;
                aJob.iDialogs = NULL;
                aJob.iNumDialogs = 0;
            }
            if (aJob.iDoneFolder != 0) {
                tg_dialogs_free(iArchive, iNumArchive);
                iArchive = aJob.iArchive;
                iNumArchive = aJob.iNumArchive;
                aJob.iArchive = NULL;
                aJob.iNumArchive = 0;
                iArchiveLoaded = ETrue;
            }
            iLoggedOut = EFalse;
            if (iScreen == EScreenChats || iScreen == EScreenStatus)
                ShowChatsL();
            break;
        case EJobHistory:
            if (iChatsAfterThread) {
                iChatsAfterThread = EFalse;
                StartDialogsL(ETrue, EFalse, ETrue);
            }
            if (rc) {
                if (!quiet)
                    ShowErrorL(aJob.iError);
                break;
            }
            if (aJob.iReadDone > iReadUpTo)
                iReadUpTo = aJob.iReadDone;
            if (aJob.iReadOut >= 0 && aJob.iPeer.id == iCur.id)
                iCur.read_out_max = aJob.iReadOut;
            if (aJob.iPeer.id != iCur.id || aJob.iPeer.kind != iCur.kind)
                break;
            {
                TInt32 keep = CurrentMessageId();
                tg_message *got = aJob.iMessages;
                TInt n = aJob.iNumMessages;
                aJob.iMessages = NULL;
                aJob.iNumMessages = 0;
                if (aJob.iOffsetId) {
                    iOlderJustLoaded = ETrue;
                    if (!n) {
                        free(got);
                        SetTitleL(iTitle);
                        CAknInformationNote *note = new (ELeave) CAknInformationNote;
                        note->ExecuteLD(T(_L("No older messages")));
                        iOlderJustLoaded = EFalse;
                        break;
                    }
                }
                MergeMessagesL(got, n);
                if (!aJob.iOffsetId) {
                    delete iPinned;
                    iPinned = aJob.iPinned ? TextLC(aJob.iPinned) : NULL;
                    if (iPinned)
                        CleanupStack::Pop(iPinned);
                    iStatus = (iCur.flags & TG_DLG_SELF) ? TG_STATUS_NONE : aJob.iStatus;
                    iStatusWhen = aJob.iStatusWhen;
                    UpdateThreadNaviL();
                }
                if (iScreen == EScreenThread)
                    ShowThreadL(keep);
            }
            break;
        case EJobSend:
        case EJobDelete:
            if (!rc && iScreen == EScreenThread) {
                DropMessages(aJob.iIds, aJob.iNumIds);
                ShowThreadL();
            }
        case EJobEdit:
        case EJobReact:
            if (rc)
                ShowErrorL(aJob.iError);
            else if (iScreen == EScreenThread)
                LoadThreadL(EFalse);
            iChatsDirty = ETrue;
            break;
        case EJobChatAction:
            if (rc) {
                ShowErrorL(aJob.iError);
            } else {
                iChatsDirty = ETrue;
                iArchiveLoaded = iArchiveLoaded && aJob.iMsgId != TG_ACT_ARCHIVE &&
                                 aJob.iMsgId != TG_ACT_UNARCHIVE;
            }
            break;
        case EJobInfo:
            if (rc) {
                ShowErrorL(aJob.iError);
            } else {
                const tg_info &in = aJob.iInfo;
                HBufC *body = HBufC::NewLC(3000);
                TPtr p = body->Des();
                if (in.username) {
                    HBufC *u = TextLC(in.username);
                    p.Append('@');
                    p.Append(*u);
                    p.Append('\n');
                    CleanupStack::PopAndDestroy(u);
                }
                if (in.phone) {
                    HBufC *ph = TextLC(in.phone);
                    p.Append('+');
                    p.Append(*ph);
                    p.Append('\n');
                    CleanupStack::PopAndDestroy(ph);
                }
                if (in.members)
                    p.AppendFormat(in.channel ? T(_L("%d subscribers\n")) : T(_L("%d members\n")),
                                   in.members);
                if (in.about) {
                    HBufC *ab = TextLC(in.about);
                    p.Append('\n');
                    p.Append(ab->Left(p.MaxLength() - p.Length()));
                    CleanupStack::PopAndDestroy(ab);
                }
                if (!p.Length())
                    p.Append(T(_L("No details")));
                HBufC *name = TextLC(in.name ? in.name : aJob.iPeer.name);
                CAknMessageQueryDialog *dlg = CAknMessageQueryDialog::NewL(*body);
                dlg->PrepareLC(R_AVKON_MESSAGE_QUERY_DIALOG);
                if (dlg->QueryHeading()) {
                    dlg->QueryHeading()->SetTextL(*name);
                    CEikImage *img = HeaderAvatarL(aJob.iPeer, *name);
                    CleanupStack::PushL(img);
                    dlg->QueryHeading()->SetHeaderImageL(img);
                    CleanupStack::Pop(img);
                }
                dlg->RunLD();
                CleanupStack::PopAndDestroy(2);
            }
            break;
        case EJobContacts:
            if (rc) {
                ShowErrorL(aJob.iError);
            } else if (iScreen == EScreenChats) {
                CDesCArrayFlat *names = new (ELeave) CDesCArrayFlat(32);
                CleanupStack::PushL(names);
                for (TInt i = 0; i < aJob.iNumContacts; i++) {
                    HBufC *n = TextLC(aJob.iContacts[i].name);
                    names->AppendL(*n);
                    CleanupStack::PopAndDestroy(n);
                }
                TInt pick = PickL(T(_L("Contacts")), names);
                CleanupStack::PopAndDestroy(names);
                if (pick >= 0)
                    OpenChatWithL(&aJob.iContacts[pick]);
            }
            break;
        case EJobResolve:
            if (rc)
                ShowErrorL(aJob.iError);
            else if (iScreen == EScreenChats)
                OpenChatWithL(&aJob.iResolved);
            break;
        case EJobPwHint:
            AskPasswordL(rc ? "" : aJob.iCodeType);
            break;
        case EJobPassword:
            if (rc) {
                ShowErrorL(aJob.iError);
                AskPasswordL(iCodeInfo);
            } else {
                ShowStatusL(T(_L("Logged in\nLoading chats...")));
                StartDialogsL(EFalse, ETrue);
            }
            break;
        case EJobForward:
            if (rc) {
                ShowErrorL(aJob.iError);
            } else {
                CAknConfirmationNote *note = new (ELeave) CAknConfirmationNote;
                note->ExecuteLD(T(_L("Forwarded")));
            }
            break;
        case EJobSendCode:
            if (rc) {
                ShowErrorL(aJob.iError);
                ShowLoggedOutL();
            } else if (!strcmp(aJob.iCodeType, "authorized")) {
                StartDialogsL(EFalse, ETrue);
            } else {
                strcpy(iCodeInfo, aJob.iCodeType);
                AskCodeL();
            }
            break;
        case EJobQrLogin:
            if (!iQrActive)
                break;
            if (rc == TG_OK) {
                ShowQrL(aJob.iText);
                iQrTimer->Cancel();
                iQrTimer->Start(3000000, 3000000, TCallBack(QrTick, this));
                break;
            }
            iQrActive = EFalse;
            if (rc == TG_QR_DONE) {
                ShowStatusL(T(_L("Logged in\nLoading chats...")));
                StartDialogsL(EFalse, ETrue);
            } else if (rc == TG_NEED_PASSWORD) {
                StartPasswordL();
            } else {
                ShowErrorL(aJob.iError);
                ShowLoggedOutL();
            }
            break;
        case EJobResendCode:
            if (rc)
                ShowErrorL(aJob.iError);
            else
                strcpy(iCodeInfo, aJob.iCodeType);
            AskCodeL();
            break;
        case EJobSignIn:
            if (rc == TG_NEED_PASSWORD) {
                StartPasswordL();
            } else if (rc) {
                ShowErrorL(aJob.iError);
                AskCodeL();
            } else {
                ShowStatusL(T(_L("Logged in\nLoading chats...")));
                StartDialogsL(EFalse, ETrue);
            }
            break;
        case EJobLogout:
            tg_dialogs_free(iDialogs, iNumDialogs);
            iDialogs = NULL;
            iNumDialogs = 0;
            tg_dialogs_free(iArchive, iNumArchive);
            iArchive = NULL;
            iNumArchive = 0;
            iArchiveLoaded = EFalse;
            tg_folders_free(iFolders, iNumFolders);
            iFolders = NULL;
            iNumFolders = 0;
            iFoldersLoaded = EFalse;
            iRows.Reset();
            iTab = 0;
            RebuildTabsL();
            ShowLoggedOutL();
            break;
        }
    }

    void LangFile(TDes &aPath)
    {
        TFileName priv;
        iEikonEnv->FsSession().PrivatePath(priv);
        aPath.Copy(_L("C:"));
        aPath.Append(priv);
        aPath.Append(_L("lang.txt"));
    }

    void ApplyLangL()
    {
        TInt setting = 0;
        TFileName path;
        LangFile(path);
        RFile f;
        if (f.Open(iEikonEnv->FsSession(), path, EFileRead) == KErrNone) {
            TBuf8<4> b;
            f.Read(b);
            f.Close();
            if (b.Length())
                setting = b[0] - '0';
        }
        if (setting >= 1 && setting <= 3) {
            gLang = setting - 1;
        } else {
            TLanguage l = User::Language();
            gLang = l == ELangRussian ? 1 : l == ELangUkrainian ? 2 : 0;
        }
        tg_set_lang(gLang);
    }

    void ChooseLanguageL()
    {
        CDesCArrayFlat *names = new (ELeave) CDesCArrayFlat(4);
        CleanupStack::PushL(names);
        names->AppendL(T(_L("Automatic")));
        names->AppendL(_L("English"));
        names->AppendL(_L("\x0420\x0443\x0441\x0441\x043A\x0438\x0439"));
        names->AppendL(_L("\x0423\x043A\x0440\x0430\x0457\x043D\x0441\x044C\x043A\x0430"));
        TInt pick = PickL(T(_L("Language")), names);
        CleanupStack::PopAndDestroy(names);
        if (pick < 0)
            return;
        TFileName path;
        LangFile(path);
        RFile f;
        if (f.Replace(iEikonEnv->FsSession(), path, EFileWrite) == KErrNone) {
            TBuf8<2> b;
            b.Append(TChar('0' + pick));
            f.Write(b);
            f.Close();
        }
        ApplyLangL();
        RebuildTabsL();
        if (iScreen == EScreenThread)
            LoadThreadL(EFalse);
        else if (iScreen == EScreenChats)
            StartDialogsL(EFalse, EFalse);
        else if (iLoggedOut && !iQrActive)
            ShowLoggedOutL();
        SetCbaL(iScreen == EScreenThread ? R_AVKON_SOFTKEYS_OPTIONS_BACK
                                         : R_AVKON_SOFTKEYS_OPTIONS_EXIT);
    }

    void InitPrivateDirL()
    {
        RFs &fs = iEikonEnv->FsSession();
        fs.CreatePrivatePath(EDriveC);
        TFileName priv;
        User::LeaveIfError(fs.PrivatePath(priv));
        TPtr8 dir(reinterpret_cast<TUint8 *>(iPrivDir), 0, sizeof(iPrivDir) - 1);
        dir.Copy(_L8("C:"));
        for (TInt i = 0; i < priv.Length() - 1; i++)
            dir.Append(priv[i] == '\\' ? '/' : TUint8(priv[i]));
        dir.ZeroTerminate();
    }

    void StartJobL(TTgJobType aType, TBool aQuiet = EFalse)
    {
        if (iWorker->Busy()) {
            if (!aQuiet) {
                iPending = aType;
                iHasPending = ETrue;
            }
            return;
        }
        iQuiet = aQuiet;
        iWorker->Job().iType = aType;
        iWorker->Start();
        if (!aQuiet)
            SetTitleL(T(_L("Loading...")));
    }

    static TInt PollTick(TAny *aSelf)
    {
        CTgAppUi *self = static_cast<CTgAppUi *>(aSelf);
        TBool screen = self->iScreen == EScreenChats || self->iScreen == EScreenThread ||
                       self->iScreen == EScreenImage;
        if (!self->iForeground && ++self->iBgTicks % 3)
            return 0;
        if (screen && !self->iLoggedOut && !self->iInJobDone && !self->iWorker->Busy())
            TRAP_IGNORE(self->StartJobL(EJobPoll, ETrue));
        return 0;
    }

    void CachePath(char aPrefix, TInt64 aId, const char *aThumb, TDes &aPath, char *aCPath, TInt aLen)
    {
        TFileName priv;
        iEikonEnv->FsSession().PrivatePath(priv);
        char name[48];
        snprintf(name, sizeof name, "%c%lld%s.jpg", aPrefix, (long long)aId, aThumb);
        aPath.Copy(_L("C:"));
        aPath.Append(priv);
        TPtrC8 n8((const TUint8 *)name);
        TBuf<48> n16;
        n16.Copy(n8);
        aPath.Append(n16);
        snprintf(aCPath, aLen, "%s/%s", iPrivDir, name);
    }

    TBool FileExists(const TDesC &aPath)
    {
        TEntry e;
        return iEikonEnv->FsSession().Entry(aPath, e) == KErrNone && e.iSize > 0;
    }

    TInt FindAvatar(TInt64 aId) const
    {
        for (TInt i = 0; i < iAvatars.Count(); i++)
            if (iAvatars[i].iId == aId)
                return i;
        return KErrNotFound;
    }

    CGulIcon *EmojiIconL(TUint32 aCp, const TSize &aSize)
    {
        for (TInt i = 0; i < iEmoji.Count(); i++) {
            if (iEmoji[i].iId != aCp)
                continue;
            TInt side = Min(aSize.iWidth, aSize.iHeight);
            TRect dst(TPoint((aSize.iWidth - side) / 2, (aSize.iHeight - side) / 2), TSize(side, side));
            CFbsBitmap *pair[2] = {iEmoji[i].iBitmap, iEmoji[i].iMask};
            CFbsBitmap *out[2] = {NULL, NULL};
            for (TInt k = 0; k < 2; k++) {
                out[k] = new (ELeave) CFbsBitmap;
                CleanupStack::PushL(out[k]);
                User::LeaveIfError(out[k]->Create(aSize, k ? EGray256 : EColor64K));
                CFbsBitmapDevice *dev = CFbsBitmapDevice::NewL(out[k]);
                CleanupStack::PushL(dev);
                CFbsBitGc *gc = NULL;
                User::LeaveIfError(dev->CreateContext(gc));
                gc->SetBrushStyle(CGraphicsContext::ESolidBrush);
                gc->SetBrushColor(KRgbBlack);
                gc->Clear();
                gc->DrawBitmap(dst, pair[k]);
                delete gc;
                CleanupStack::PopAndDestroy(dev);
            }
            CGulIcon *icon = CGulIcon::NewL(out[0], out[1]);
            CleanupStack::Pop(2);
            return icon;
        }
        return NULL;
    }

    CGulIcon *ReactionStripL(const tg_message &aMsg, TInt aSide)
    {
        TUint32 cps[5];
        TInt n = 0;
        for (TInt r = 0; r < aMsg.react_kinds && n < 5; r++)
            if (HasEmoji(aMsg.react_cp[r]))
                cps[n++] = aMsg.react_cp[r];
        if (!n)
            return NULL;
        const TInt gap = 2;
        TSize size(n * aSide + (n - 1) * gap, aSide);
        CFbsBitmap *out[2] = {NULL, NULL};
        for (TInt k = 0; k < 2; k++) {
            out[k] = new (ELeave) CFbsBitmap;
            CleanupStack::PushL(out[k]);
            User::LeaveIfError(out[k]->Create(size, k ? EGray256 : EColor64K));
            CFbsBitmapDevice *dev = CFbsBitmapDevice::NewL(out[k]);
            CleanupStack::PushL(dev);
            CFbsBitGc *gc = NULL;
            User::LeaveIfError(dev->CreateContext(gc));
            gc->SetBrushStyle(CGraphicsContext::ESolidBrush);
            gc->SetBrushColor(KRgbBlack);
            gc->Clear();
            for (TInt i = 0; i < n; i++)
                for (TInt e = 0; e < iEmoji.Count(); e++)
                    if (iEmoji[e].iId == cps[i])
                        gc->DrawBitmap(TRect(TPoint(i * (aSide + gap), 0), TSize(aSide, aSide)),
                                       k ? iEmoji[e].iMask : iEmoji[e].iBitmap);
            delete gc;
            CleanupStack::PopAndDestroy(dev);
        }
        CGulIcon *icon = CGulIcon::NewL(out[0], out[1]);
        CleanupStack::Pop(2);
        return icon;
    }

    TBool HasEmoji(TUint32 aCp) const
    {
        for (TInt i = 0; i < iEmoji.Count(); i++)
            if (iEmoji[i].iId == aCp)
                return ETrue;
        return EFalse;
    }

    static CFbsBitmap *ScaledCopyLC(CFbsBitmap *aSrc, const TSize &aSize, TDisplayMode aMode)
    {
        CFbsBitmap *b = new (ELeave) CFbsBitmap;
        CleanupStack::PushL(b);
        User::LeaveIfError(b->Create(aSize, aMode));
        CFbsBitmapDevice *dev = CFbsBitmapDevice::NewL(b);
        CleanupStack::PushL(dev);
        CFbsBitGc *gc = NULL;
        User::LeaveIfError(dev->CreateContext(gc));
        gc->DrawBitmap(TRect(aSize), aSrc);
        delete gc;
        CleanupStack::PopAndDestroy(dev);
        return b;
    }

    CEikImage *HeaderAvatarL(const tg_dialog &aPeer, const TDesC &aName)
    {
        TSize size(20, 20);
        TInt found = aPeer.photo.type == TG_LOC_PEER_PHOTO ? FindAvatar(aPeer.photo.id) : KErrNotFound;
        CGulIcon *src = found >= 0 ? AvatarIconL(found) : LetterIconL(size, aName, aPeer.id);
        CleanupStack::PushL(src);
        CFbsBitmap *b = ScaledCopyLC(src->Bitmap(), size, EColor64K);
        CFbsBitmap *m = ScaledCopyLC(src->Mask(), size, EGray256);
        CEikImage *img = new (ELeave) CEikImage;
        img->SetPicture(b, m);
        CleanupStack::Pop(2);
        CleanupStack::PopAndDestroy(src);
        return img;
    }

    CGulIcon *AvatarIconL(TInt aIndex)
    {
        CFbsBitmap *b = new (ELeave) CFbsBitmap;
        CleanupStack::PushL(b);
        User::LeaveIfError(b->Duplicate(iAvatars[aIndex].iBitmap->Handle()));
        CFbsBitmap *m = new (ELeave) CFbsBitmap;
        CleanupStack::PushL(m);
        User::LeaveIfError(m->Duplicate(iAvatars[aIndex].iMask->Handle()));
        CGulIcon *icon = CGulIcon::NewL(b, m);
        CleanupStack::Pop(2);
        return icon;
    }

    void RequestAvatarsL()
    {
        if (!iList || !iList->HasIcons())
            return;
        TSize size = iIconSize = iList->IconSize();
        for (TInt i = 0; i < iRows.Count(); i++) {
            const tg_file_loc &loc = DialogAt(iRows[i])->photo;
            if (loc.type != TG_LOC_PEER_PHOTO || (DialogAt(iRows[i])->flags & TG_DLG_SELF) || FindAvatar(loc.id) >= 0 ||
                iAvatarAsked.Find(loc.id) >= 0)
                continue;
            iAvatarAsked.AppendL(loc.id);
            TFileName path;
            char cpath[128];
            CachePath('a', loc.id, "", path, cpath, sizeof cpath);
            if (FileExists(path))
                iDecoder->AddL(path, loc.id, 0, size);
            else
                iWanted.AppendL(loc);
        }
        KickDownloadsL();
    }

    void KickDownloadsL()
    {
        if (!iWanted.Count() || iWorker->Busy() || iInJobDone)
            return;
        TTgJob &j = iWorker->Job();
        j.iNumLocs = 0;
        while (iWanted.Count() && j.iNumLocs < 4) {
            TFileName path;
            j.iLocs[j.iNumLocs] = iWanted[0];
            if (iWanted[0].type == TG_LOC_PEER_PHOTO)
                CachePath('a', iWanted[0].id, "", path, j.iPaths[j.iNumLocs], sizeof j.iPaths[0]);
            else
                CachePath('t', iWanted[0].id, iWanted[0].thumb, path, j.iPaths[j.iNumLocs],
                          sizeof j.iPaths[0]);
            iWanted.Remove(0);
            j.iNumLocs++;
        }
        StartJobL(EJobAvatars, ETrue);
    }

    void OpenMediaL(const tg_message &aMsg)
    {
        TFileName path;
        TTgJob &j = iWorker->Job();
        iViewKey = aMsg.media.id;
        if (iScreen == EScreenThread && iList)
            iViewIndex = iList->CurrentIndex();
        CachePath('m', aMsg.media.id, aMsg.media.thumb, path, j.iMediaPath, sizeof j.iMediaPath);
        SetTitleL(T(_L("Loading picture...")));
        if (FileExists(path)) {
            iDecoder->AddL(path, iViewKey, 1, ClientRect().Size());
        } else {
            j.iMediaLoc = aMsg.media;
            j.iMediaPurpose = 0;
            StartJobL(EJobMedia);
        }
        iFullAsked = EFalse;
    }

    void SetAlbumFor(const tg_message &aMsg)
    {
        iAlbum.Reset();
        iAlbumPos = 0;
        if (!aMsg.grouped_id)
            return;
        for (TInt i = 0; i < iNumMessages; i++)
            if (iMessages[i].grouped_id == aMsg.grouped_id &&
                iMessages[i].media.type != TG_LOC_NONE) {
                if (iMessages[i].id == aMsg.id || (!iAlbum.Count() && !aMsg.media.type))
                    iAlbumPos = iAlbum.Count();
                iAlbum.Append(i);
            }
        if (iAlbum.Count() < 2)
            iAlbum.Reset();
    }

    void AlbumStepL(TInt aDelta)
    {
        if (iAlbum.Count() < 2)
            return;
        TInt pos = iAlbumPos + aDelta;
        if (pos < 0 || pos >= iAlbum.Count())
            return;
        iAlbumPos = pos;
        OpenMediaL(iMessages[iAlbum[pos]]);
    }

    void ZoomL(TInt aDelta)
    {
        if (!iImage)
            return;
        iImage->SetZoom(iImage->Zoom() + aDelta);
        if (iImage->Zoom() > 1)
            FetchFullL();
    }

    tg_message *ViewedMessage()
    {
        if (iScreen == EScreenImage && iAlbum.Count())
            return &iMessages[iAlbum[iAlbumPos]];
        TInt row = iScreen == EScreenImage ? iViewIndex : iList ? iList->CurrentIndex() : -1;
        if (iScreen != EScreenImage && iScreen != EScreenThread)
            return NULL;
        if (row < 0 || row >= iMsgRows.Count() || iMsgRows[row] < 0)
            return NULL;
        return &iMessages[iMsgRows[row]];
    }

    void FetchFullL()
    {
        tg_message *m = ViewedMessage();
        if (iFullAsked || !m || m->full.type != TG_LOC_PHOTO)
            return;
        iFullAsked = ETrue;
        TFileName path;
        TTgJob &j = iWorker->Job();
        CachePath('f', m->full.id, m->full.thumb, path, j.iMediaPath, sizeof j.iMediaPath);
        SetTitleL(T(_L("Loading full size...")));
        if (FileExists(path)) {
            TSize t = ClientRect().Size();
            iDecoder->AddL(path, m->full.id, 2, TSize(t.iWidth * 4, t.iHeight * 4));
        } else {
            j.iMediaLoc = m->full;
            j.iMediaPurpose = 1;
            StartJobL(EJobMedia);
        }
    }

    static void SafeName(const char *aIn, char *aOut, TInt aLen)
    {
        TInt n = 0;
        for (; *aIn && n < aLen - 1; aIn++) {
            unsigned char ch = (unsigned char)*aIn;
            aOut[n++] = ch < 32 || ch > 126 || strchr("\\/:*?\"<>|", ch) ? '_' : ch;
        }
        aOut[n] = 0;
    }

    void OpenExternalL(const TDesC &aPath)
    {
        if (!iDocHandler)
            iDocHandler = CDocumentHandler::NewL();
        TDataType type;
        TInt err = iDocHandler->OpenFileL(aPath, type);
        if (err != KErrNone) {
            TBuf<64> t;
            t.Format(T(_L("No app can open this file (%d)")), err);
            InfoL(t);
        }
    }

    tg_message *PickInGroupL(tg_message *aMsg)
    {
        TInt first = aMsg - iMessages, last = first;
        while (first > 0 && iMessages[first - 1].grouped_id == aMsg->grouped_id)
            first--;
        while (last + 1 < iNumMessages && iMessages[last + 1].grouped_id == aMsg->grouped_id)
            last++;
        if (first == last)
            return aMsg;
        CDesCArrayFlat *names = new (ELeave) CDesCArrayFlat(last - first + 1);
        CleanupStack::PushL(names);
        for (TInt i = first; i <= last; i++) {
            if (iMessages[i].file_name[0]) {
                names->AppendL(*TextLC(iMessages[i].file_name));
                CleanupStack::PopAndDestroy();
            } else {
                TBuf<32> t;
                t.Format(T(_L("Photo %d")), i - first + 1);
                names->AppendL(t);
            }
        }
        TInt pick = PickL(T(_L("Choose a file")), names);
        CleanupStack::PopAndDestroy(names);
        return pick >= 0 ? &iMessages[first + pick] : NULL;
    }

    void SaveMediaL(TBool aOpen = EFalse)
    {
        iOpenAfterSave = aOpen;
        tg_message *m = ViewedMessage();
        if (m && iScreen == EScreenThread && m->grouped_id) {
            m = PickInGroupL(m);
            if (!m)
                return;
        }
        if (!m || m->save_kind == TG_SAVE_NONE || m->full.type == TG_LOC_NONE) {
            InfoL(T(_L("Nothing to save in this message")));
            return;
        }
        static const char *const folders[] = {"", "Images", "Videos", "Music", "Others"};
        TVolumeInfo vol;
        TBool card = iEikonEnv->FsSession().Volume(vol, EDriveE) == KErrNone;
        char name[80], dir[64], cpath[128];
        if (m->file_name[0]) {
            char safe[64];
            SafeName(m->file_name, safe, sizeof safe);
            snprintf(name, sizeof name, "%lld_%s", (long long)(m->full.id % 100000), safe);
        } else {
            snprintf(name, sizeof name, "IMG_%lld.jpg", (long long)m->full.id);
        }
        snprintf(dir, sizeof dir, "%s/%s/Telegram", card ? "E:" : "C:/Data",
                 folders[m->save_kind]);
        snprintf(cpath, sizeof cpath, "%s/%s", dir, name);
        TPtrC8 p8((const TUint8 *)cpath);
        iSaveName.Copy(p8);
        for (TInt i = 0; i < iSaveName.Length(); i++)
            if (iSaveName[i] == '/')
                iSaveName[i] = '\\';
        TParsePtrC parse(iSaveName);
        iEikonEnv->FsSession().MkDirAll(parse.DriveAndPath());
        TFileName cached;
        char unused[128];
        CachePath('f', m->full.id, m->full.thumb, cached, unused, sizeof unused);
        if (m->full.type == TG_LOC_PHOTO && FileExists(cached)) {
            CFileMan *fm = CFileMan::NewL(iEikonEnv->FsSession());
            TInt err = fm->Copy(cached, iSaveName, CFileMan::EOverWrite);
            delete fm;
            if (err == KErrNone && aOpen) {
                OpenExternalL(iSaveName);
                return;
            }
            HBufC *msg = HBufC::NewLC(iSaveName.Length() + 32);
            msg->Des().Copy(err == KErrNone ? T(_L("Saved to ")) : T(_L("Could not save to ")));
            msg->Des().Append(iSaveName);
            InfoL(*msg);
            CleanupStack::PopAndDestroy(msg);
            return;
        }
        if (aOpen && FileExists(iSaveName)) {
            OpenExternalL(iSaveName);
            return;
        }
        TTgJob &j = iWorker->Job();
        j.iMediaLoc = m->full;
        j.iMediaPurpose = 2;
        strncpy(j.iMediaPath, cpath, sizeof j.iMediaPath - 1);
        j.iMediaPath[sizeof j.iMediaPath - 1] = 0;
        SetTitleL(T(_L("Saving...")));
        StartJobL(EJobMedia);
    }

    void OpenCameraL()
    {
        if (iScreen != EScreenThread)
            return;
        iViewIndex = iList ? iList->CurrentIndex() : -1;
        CTgCamera *cam = NULL;
        TRAPD(err, cam = CTgCamera::NewL(ClientRect(), *this));
        if (err != KErrNone) {
            CameraFailedL(err);
            return;
        }
        Retire(iList);
        iList = NULL;
        iCameraCtl = cam;
        AddToStackL(iCameraCtl);
        iScreen = EScreenCamera;
        SetTitleL(T(_L("Camera")));
        SetCbaL(R_TG_SOFTKEYS_CAPTURE_BACK);
    }

    void CameraFailedL(TInt aError)
    {
        TBuf<64> t;
        t.Format(T(_L("Camera not available (%d)")), aError);
        InfoL(t);
        if (iScreen == EScreenCamera)
            ShowThreadL();
    }

    void PhotoTakenL(HBufC8 *aData)
    {
        delete iCaptured;
        iCaptured = aData;
        iAfterCapture->CallBack();
    }

    static TInt AfterCapture(TAny *aSelf)
    {
        CTgAppUi *self = static_cast<CTgAppUi *>(aSelf);
        TRAPD(err, self->SendCapturedL());
        if (err != KErrNone)
            TRAP_IGNORE(self->CameraFailedL(err));
        return 0;
    }

    void SendCapturedL()
    {
        HBufC8 *data = iCaptured;
        iCaptured = NULL;
        if (!data)
            return;
        CleanupStack::PushL(data);
        TVolumeInfo vol;
        RFs &fs = iEikonEnv->FsSession();
        TBool card = fs.Volume(vol, EDriveE) == KErrNone;
        TFileName path(card ? _L("E:\\Images\\Telegram\\") : _L("C:\\Data\\Images\\Telegram\\"));
        fs.MkDirAll(path);
        TTime now;
        now.HomeTime();
        TDateTime dt = now.DateTime();
        path.AppendFormat(_L("TG_%04d%02d%02d_%02d%02d%02d.jpg"), dt.Year(), dt.Month() + 1,
                          dt.Day() + 1, dt.Hour(), dt.Minute(), dt.Second());
        RFile f;
        User::LeaveIfError(f.Replace(fs, path, EFileWrite));
        TInt err = f.Write(*data);
        f.Close();
        CleanupStack::PopAndDestroy(data);
        User::LeaveIfError(err);
        ShowThreadL();
        StartSendL(path, ETrue);
    }

    void SendPickedL(TBool aPhoto)
    {
        if (iScreen != EScreenThread)
            return;
        TFileName path;
        TVolumeInfo vol;
        TBool card = iEikonEnv->FsSession().Volume(vol, EDriveE) == KErrNone;
        TFileName folder(card ? _L("E:\\") : _L("C:\\Data\\"));
        if (aPhoto) {
            folder.Append(_L("Images\\"));
            TEntry e;
            if (iEikonEnv->FsSession().Entry(folder, e) != KErrNone)
                folder.SetLength(folder.Length() - 7);
        }
        path = folder;
        if (CAknFileSelectionDialog::RunDlgLD(path, folder,
                                              aPhoto ? T(_L("Select photo")) : T(_L("Select file")))) {
            if (path.Locate(':') < 0)
                path.Insert(0, folder);
        } else {
            path.Zero();
        }
        if (!path.Length() || !FileExists(path))
            return;
        StartSendL(path, aPhoto);
    }

    static void ScanPhotosL(RFs &aFs, const TDesC &aRoot, CDesCArray &aPaths, RArray<TInt64> &aTimes)
    {
        CDirScan *scan = CDirScan::NewLC(aFs);
        TRAPD(err, scan->SetScanDataL(aRoot, KEntryAttNormal, ESortNone, CDirScan::EScanDownTree));
        if (err != KErrNone) {
            CleanupStack::PopAndDestroy(scan);
            return;
        }
        for (;;) {
            CDir *dir = NULL;
            TRAP(err, scan->NextL(dir));
            if (err != KErrNone || !dir)
                break;
            CleanupStack::PushL(dir);
            if (scan->FullPath().Find(_L("\\_")) < 0)
                for (TInt i = 0; i < dir->Count(); i++) {
                    const TEntry &e = (*dir)[i];
                    TParsePtrC p(e.iName);
                    if (p.Ext().CompareF(_L(".jpg")) && p.Ext().CompareF(_L(".jpeg")) &&
                        p.Ext().CompareF(_L(".png")))
                        continue;
                    TFileName full(scan->FullPath());
                    full.Append(e.iName);
                    aPaths.AppendL(full);
                    aTimes.AppendL(e.iModified.Int64());
                }
            CleanupStack::PopAndDestroy(dir);
        }
        CleanupStack::PopAndDestroy(scan);
    }

    void SendPhotosL()
    {
        if (iScreen != EScreenThread)
            return;
        RFs &fs = iEikonEnv->FsSession();
        CDesCArrayFlat *paths = new (ELeave) CDesCArrayFlat(32);
        CleanupStack::PushL(paths);
        RArray<TInt64> times;
        CleanupClosePushL(times);
        ScanPhotosL(fs, _L("E:\\Images\\"), *paths, times);
        ScanPhotosL(fs, _L("C:\\Data\\Images\\"), *paths, times);
        if (!paths->Count()) {
            CleanupStack::PopAndDestroy(2);
            SendPickedL(ETrue);
            return;
        }
        RArray<TInt> order;
        CleanupClosePushL(order);
        for (TInt i = 0; i < paths->Count(); i++) {
            TInt pos = 0;
            while (pos < order.Count() && times[order[pos]] >= times[i])
                pos++;
            if (pos < 100)
                order.InsertL(i, pos);
            if (order.Count() > 100)
                order.Remove(100);
        }
        CDesCArrayFlat *items = new (ELeave) CDesCArrayFlat(order.Count());
        CleanupStack::PushL(items);
        for (TInt i = 0; i < order.Count(); i++) {
            TBuf<128> item, when;
            TTime(times[order[i]]).FormatL(when, _L("%F%D.%M %H:%T"));
            item.Format(_L("1\t%S  "), &when);
            TPtrC name = TParsePtrC((*paths)[order[i]]).NameAndExt();
            item.Append(name.Left(item.MaxLength() - item.Length()));
            items->AppendL(item);
        }
        CArrayFixFlat<TInt> *sel = new (ELeave) CArrayFixFlat<TInt>(8);
        CleanupStack::PushL(sel);
        CAknListQueryDialog *dlg = new (ELeave) CAknListQueryDialog(sel);
        dlg->PrepareLC(R_TG_PHOTOS_QUERY);
        dlg->SetItemTextArray(items);
        dlg->SetOwnershipType(ELbmDoesNotOwnItemArray);
        dlg->QueryHeading()->SetTextL(T(_L("Select photos")));
        if (dlg->RunLD() && sel->Count()) {
            TInt n = sel->Count();
            if (n > 10) {
                InfoL(T(_L("Up to 10 photos per album, sending the first 10")));
                n = 10;
            }
            if (n == 1) {
                StartSendL((*paths)[order[(*sel)[0]]], ETrue);
            } else {
                TTgJob &j = iWorker->Job();
                j.iText[0] = 0;
                if (AskYesNoL(T(_L("Add a caption?"))) &&
                    !QueryUtf8L(T(_L("Caption")), j.iText, sizeof j.iText, R_TG_PROSE_QUERY))
                    j.iText[0] = 0;
                for (TInt i = 0; i < n; i++)
                    j.iAlbumPaths[i] = (*paths)[order[(*sel)[n - 1 - i]]];
                j.iAlbumCount = n;
                j.iPeer = iCur;
                j.iProgress = 0;
                j.iAlbumIndex = 0;
                iAtBottom = ETrue;
                StartJobL(EJobSendFile);
                iProgressTimer->Cancel();
                iProgressTimer->Start(1000000, 1000000, TCallBack(ProgressTick, this));
            }
        }
        CleanupStack::PopAndDestroy(5);
    }

    void StartSendL(const TDesC &aPath, TBool aPhoto)
    {
        TTgJob &j = iWorker->Job();
        j.iText[0] = 0;
        if (AskYesNoL(T(_L("Add a caption?"))) &&
            !QueryUtf8L(T(_L("Caption")), j.iText, sizeof j.iText, R_TG_PROSE_QUERY))
            j.iText[0] = 0;
        j.iSendPath = aPath;
        j.iSendAsPhoto = aPhoto;
        j.iAlbumCount = 0;
        j.iPeer = iCur;
        j.iProgress = 0;
        iAtBottom = ETrue;
        StartJobL(EJobSendFile);
        iProgressTimer->Cancel();
        iProgressTimer->Start(1000000, 1000000, TCallBack(ProgressTick, this));
    }

    static TInt ProgressTick(TAny *aSelf)
    {
        CTgAppUi *self = static_cast<CTgAppUi *>(aSelf);
        if (self->iWorker->Busy() && self->iWorker->Running().iType == EJobSendFile) {
            TBuf<32> t;
            const TTgJob &j = self->iWorker->Running();
            TInt n = j.iAlbumCount ? j.iAlbumCount : 1;
            t.Format(T(_L("Sending %d%%")), (j.iAlbumIndex * 100 + j.iProgress) / n);
            TRAP_IGNORE(self->SetTitleL(t));
        }
        return 0;
    }

    void ShowImageL(CFbsBitmap *aBitmap)
    {
        CloseMsgView();
        CleanupStack::PushL(aBitmap);
        Retire(iList);
        iList = NULL;
        Retire(iImage);
        iImage = CTgImage::NewL(ClientRect(), *this, aBitmap);
        CleanupStack::Pop(aBitmap);
        AddToStackL(iImage);
        iScreen = EScreenImage;
        if (iAlbum.Count() > 1) {
            TBuf<80> t;
            t.Format(_L("%d/%d  "), iAlbumPos + 1, iAlbum.Count());
            t.Append(iTitle.Left(t.MaxLength() - t.Length()));
            SetTitleL(t);
        } else {
            SetTitleL(iTitle);
        }
        SetCbaL(R_AVKON_SOFTKEYS_OPTIONS_BACK);
    }

    void StartDialogsL(TBool aQuiet, TBool aFolders, TBool aBoth = EFalse)
    {
        if (aQuiet && iWorker->Busy())
            return;
        TTgJob &j = iWorker->Job();
        if (aBoth)
            j.iFolder = 2;
        else if (iTab == ArchiveTab())
            j.iFolder = 1;
        else if (!iFoldersLoaded || !iArchiveLoaded || !iFolders[iTab].all)
            j.iFolder = 2;
        else
            j.iFolder = 0;
        j.iWithFolders = aFolders || !iFoldersLoaded;
        StartJobL(EJobDialogs, aQuiet);
    }

    TInt ArchiveTab() const { return Max(iNumFolders, 1); }

    TInt AllTab() const
    {
        for (TInt i = 0; i < iNumFolders; i++)
            if (iFolders[i].all)
                return i;
        return 0;
    }

    void RebuildTabsL()
    {
        TBool shown = iTabsShown;
        HideTabs();
        delete iTabDeco;
        iTabDeco = NULL;
        iTabDeco = iNavi->CreateTabGroupL();
        CAknTabGroup *tabs = static_cast<CAknTabGroup *>(iTabDeco->DecoratedControl());
        tabs->SetTabFixedWidthL(EAknTabWidthWithThreeLongTabs);
        if (!iNumFolders)
            tabs->AddTabL(0, T(_L("All chats")));
        for (TInt i = 0; i < iNumFolders; i++) {
            HBufC *t = iFolders[i].all ? T(_L("All chats")).AllocLC() : TextLC(iFolders[i].title);
            tabs->AddTabL(i, *t);
            CleanupStack::PopAndDestroy(t);
        }
        tabs->AddTabL(ArchiveTab(), T(_L("Archive")));
        if (iTab > ArchiveTab())
            iTab = 0;
        tabs->SetActiveTabByIndex(iTab);
        iTabCounts.Reset();
        UpdateTabBadgesL();
        if (shown)
            ShowTabsL();
    }

    void UpdateTabBadgesL()
    {
        if (!iTabDeco)
            return;
        CAknTabGroup *tabs = static_cast<CAknTabGroup *>(iTabDeco->DecoratedControl());
        TInt tabCount = Max(iNumFolders, 1);
        TBool changed = EFalse;
        for (TInt t = 0; t < tabCount; t++) {
            TBool all = !iNumFolders || iFolders[t].all;
            TInt count = 0;
            for (TInt i = 0; i < (all ? iNumDialogs : iNumDialogs + iNumArchive); i++) {
                const tg_dialog *d = DialogAt(i);
                if ((d->unread || (d->flags & TG_DLG_UNREAD_MARK)) && !(d->flags & TG_DLG_MUTED) &&
                    (all || tg_folder_has(&iFolders[t], d)))
                    count++;
            }
            if (t < iTabCounts.Count() && iTabCounts[t] == count)
                continue;
            while (iTabCounts.Count() <= t)
                iTabCounts.AppendL(-1);
            iTabCounts[t] = count;
            HBufC *name = iNumFolders && !iFolders[t].all ? TextLC(iFolders[t].title)
                                                          : T(_L("All chats")).AllocLC();
            HBufC *title = HBufC::NewLC(name->Length() + 12);
            title->Des().Copy(*name);
            if (count)
                title->Des().AppendFormat(_L(" (%d)"), count);
            tabs->ReplaceTabL(t, *title);
            CleanupStack::PopAndDestroy(2);
            changed = ETrue;
        }
        if (changed) {
            tabs->SetActiveTabByIndex(iTab);
            tabs->DrawDeferred();
        }
    }

    void ShowTabsL()
    {
        HideNaviLabel();
        if (!iTabsShown && iTabDeco) {
            iNavi->PushL(*iTabDeco);
            iTabsShown = ETrue;
        }
    }

    void UpdateThreadNaviL()
    {
        TBuf<120> label;
        if (iCur.unread > 0) {
            label.Format(T(_L("%d new")),
                         iCur.unread);
        } else if (iStatus != TG_STATUS_NONE) {
            if (iStatus == TG_STATUS_ONLINE) {
                label = T(_L("online"));
            } else if (iStatus == TG_STATUS_SEEN) {
                TBuf<16> when;
                TInt day = LocalDay(iStatusWhen), today = LocalDay(NowUnix());
                if (day == today)
                    TimeText(iStatusWhen, when);
                else if (day == today - 1)
                    when = T(_L("yesterday"));
                else
                    DayText(iStatusWhen, when);
                label = T(_L("last seen "));
                label.Append(when);
            } else {
                label = iStatus == TG_STATUS_RECENTLY ? T(_L("last seen recently"))
                        : iStatus == TG_STATUS_WEEK   ? T(_L("last seen within a week"))
                                                      : T(_L("last seen within a month"));
            }
        }
        if (label.Length())
            ShowNaviLabelL(label);
        else
            HideNaviLabel();
    }

    void ShowNaviLabelL(const TDesC &aText)
    {
        HideNaviLabel();
        iNaviLabel = iNavi->CreateNavigationLabelL(aText);
        iNavi->PushL(*iNaviLabel);
    }

    void HideNaviLabel()
    {
        if (iNaviLabel) {
            iNavi->Pop(iNaviLabel);
            delete iNaviLabel;
            iNaviLabel = NULL;
        }
    }

    void HideTabs()
    {
        if (iTabsShown) {
            iNavi->Pop(iTabDeco);
            iTabsShown = EFalse;
        }
    }

    void SwitchTabL(TInt aDelta)
    {
        TInt t = iTab + aDelta;
        if (t < 0 || t > ArchiveTab())
            return;
        iTab = t;
        static_cast<CAknTabGroup *>(iTabDeco->DecoratedControl())->SetActiveTabByIndex(t);
        iSelId = 0;
        iSelKind = -1;
        ShowChatsL();
        if (t == ArchiveTab())
            StartDialogsL(iArchiveLoaded, EFalse);
    }

    TBool InTab(const tg_dialog &aDialog) const
    {
        if (iTab == ArchiveTab() || !iNumFolders)
            return ETrue;
        return tg_folder_has(&iFolders[iTab], &aDialog);
    }

    tg_dialog *DialogAt(TInt aRow) const
    {
        if (aRow >= 0 && aRow < iNumDialogs)
            return &iDialogs[aRow];
        aRow -= iNumDialogs;
        return aRow >= 0 && aRow < iNumArchive ? &iArchive[aRow] : NULL;
    }

    TInt OldUnread(const tg_dialog &aNew) const
    {
        for (TInt i = 0; i < iNumDialogs + iNumArchive; i++) {
            const tg_dialog *d = DialogAt(i);
            if (d->id == aNew.id && d->kind == aNew.kind)
                return d->unread;
        }
        return iNumDialogs + iNumArchive ? 0 : -1;
    }

    void NotifyL(const TTgJob &aJob)
    {
        TInt count = 0, i;
        TBool away = UserAway();
        HBufC *text = HBufC::NewLC(300);
        TPtr p = text->Des();
        const tg_dialog *first = NULL;
        for (i = 0; i < aJob.iNumDialogs + aJob.iNumArchive; i++) {
            const tg_dialog &d = i < aJob.iNumDialogs ? aJob.iDialogs[i]
                                                      : aJob.iArchive[i - aJob.iNumDialogs];
            TInt old = OldUnread(d);
            if (old < 0 || d.unread <= old || (d.flags & (TG_DLG_MUTED | TG_DLG_SELF)) || d.out)
                continue;
            if (!away && (iScreen == EScreenThread || iScreen == EScreenImage) &&
                d.id == iCur.id && d.kind == iCur.kind)
                continue;
            if (!count++)
                first = &d;
            HBufC *name = TextLC(d.name);
            if (p.Length() && p.Length() + 2 < p.MaxLength())
                p.Append(_L(", "));
            p.Append(name->Left(Max(0, p.MaxLength() - p.Length() - 1)));
            CleanupStack::PopAndDestroy(name);
        }
        if (!count) {
            CleanupStack::PopAndDestroy(text);
            return;
        }
        if (!away) {
            if (!iVibra)
                TRAP_IGNORE(iVibra = CHWRMVibra::NewL());
            if (iVibra)
                TRAP_IGNORE(iVibra->StartVibraL(150));
            CleanupStack::PopAndDestroy(text);
            return;
        }
        HBufC *msg = HBufC::NewLC(400);
        TPtr mp = msg->Des();
        if (count == 1) {
            HBufC *body = TextLC(first->text);
            mp.Copy(p.Left(80));
            mp.Append(_L(": "));
            mp.Append(body->Left(mp.MaxLength() - mp.Length()));
            CleanupStack::PopAndDestroy(body);
        } else {
            mp.Format(T(_L("%d chats with new messages: ")), count);
            mp.Append(p.Left(mp.MaxLength() - mp.Length()));
        }
        CAknGlobalNote *note = CAknGlobalNote::NewLC();
        note->ShowNoteL(EAknGlobalInformationNote, mp);
        CleanupStack::PopAndDestroy(note);
        if (!iVibra)
            TRAP_IGNORE(iVibra = CHWRMVibra::NewL());
        if (iVibra)
            TRAP_IGNORE(iVibra->StartVibraL(400));
        CleanupStack::PopAndDestroy(2);
    }

    void RefreshL()
    {
        if (!iForeground)
            StartDialogsL(ETrue, EFalse, ETrue);
        else if (iScreen == EScreenChats)
            StartDialogsL(ETrue, EFalse);
        else if (iScreen == EScreenThread) {
            iChatsAfterThread = ETrue;
            LoadThreadL(ETrue);
        }
    }

    TBool UserAway()
    {
        if (!iForeground || User::InactivityTime().Int() >= 30)
            return ETrue;
        RAknKeyLock lock;
        TBool locked = EFalse;
        if (lock.Connect() == KErrNone) {
            locked = lock.IsKeyLockEnabled();
            lock.Close();
        }
        return locked;
    }

    void HandleForegroundEventL(TBool aForeground)
    {
        CAknAppUi::HandleForegroundEventL(aForeground);
        iForeground = aForeground;
        if (aForeground && !iLoggedOut && !iWorker->Busy())
            RefreshL();
    }

    void SetTitleL(const TDesC &aTitle)
    {
        CAknTitlePane *title = static_cast<CAknTitlePane *>(
            StatusPane()->ControlL(TUid::Uid(EEikStatusPaneUidTitle)));
        title->SetTextL(aTitle);
    }

    void SetCbaL(TInt aResource)
    {
        if (iMsgView)
            return;
        CEikButtonGroupContainer *cba = Cba();
        cba->SetCommandSetL(aResource);
        cba->MakeCommandVisible(aResource == R_TG_SOFTKEYS_CAPTURE_BACK ? EAknSoftkeyOk
                                                                       : EAknSoftkeyOptions,
                                ETrue);
        if (aResource == R_AVKON_SOFTKEYS_OPTIONS_EXIT) {
            cba->SetCommandL(EAknSoftkeyOptions, T(_L("Options")));
            cba->SetCommandL(EAknSoftkeyExit, T(_L("Hide")));
        } else if (aResource == R_AVKON_SOFTKEYS_OPTIONS_BACK) {
            cba->SetCommandL(EAknSoftkeyOptions, T(_L("Options")));
            cba->SetCommandL(EAknSoftkeyBack, T(_L("Back")));
        } else if (aResource == R_TG_SOFTKEYS_CAPTURE_BACK) {
            cba->SetCommandL(EAknSoftkeyOk, T(_L("Capture")));
            cba->SetCommandL(EAknSoftkeyBack, T(_L("Back")));
        }
        cba->DrawDeferred();
    }

    void Retire(CCoeControl *aControl)
    {
        if (!aControl)
            return;
        RemoveFromStack(aControl);
        aControl->MakeVisible(EFalse);
        if (iDead.Append(aControl) != KErrNone)
            delete aControl;
        else
            iReaper->CallBack();
    }

    static TInt Reap(TAny *aSelf)
    {
        CTgAppUi *self = static_cast<CTgAppUi *>(aSelf);
        for (TInt i = 0; i < self->iDead.Count(); i++)
            delete self->iDead[i];
        self->iDead.Reset();
        return 0;
    }

    CTgList *ListL(TScreen aScreen, TBool aIcons)
    {
        if (iList && iScreen == aScreen && iList->HasIcons() == aIcons) {
            iList->ResetL();
            return iList;
        }
        CloseMsgView();
        Retire(iList);
        Retire(iQr);
        Retire(iImage);
        Retire(iCameraCtl);
        iQr = NULL;
        iImage = NULL;
        iCameraCtl = NULL;
        iList = CTgList::NewL(ClientRect(), *this, aIcons);
        AddToStackL(iList);
        iScreen = aScreen;
        return iList;
    }

    void ShowStatusL(const TDesC &aText)
    {
        HideTabs();
        HideNaviLabel();
        CTgList *list = ListL(EScreenStatus, EFalse);
        list->SetEmptyTextL(aText);
        list->DoneL(-1);
        iTitle = _L("Telegram");
        SetTitleL(iTitle);
        SetCbaL(R_AVKON_SOFTKEYS_OPTIONS_EXIT);
    }

    void ShowErrorL(const char *aError)
    {
        HBufC *t = TextLC(aError);
        HBufC *msg = HBufC::NewLC(t->Length() + 16);
        msg->Des().Copy(T(_L("Error: ")));
        msg->Des().Append(*t);
        InfoL(*msg);
        CleanupStack::PopAndDestroy(2);
    }

    void ShowLoggedOutL()
    {
        iLoggedOut = ETrue;
        ShowStatusL(T(_L("Not logged in\nPress the select key to log in with a QR code")));
    }

    void StartPasswordL()
    {
        iLoggedOut = ETrue;
        iQrActive = EFalse;
        iQrTimer->Cancel();
        ShowStatusL(T(_L("Two-step verification")));
        StartJobL(EJobPwHint);
    }

    void AskPasswordL(const char *aHint)
    {
        strncpy(iCodeInfo, aHint, sizeof iCodeInfo - 1);
        iCodeInfo[sizeof iCodeInfo - 1] = 0;
        HBufC *hint = TextLC(aHint);
        HBufC *prompt = HBufC::NewLC(hint->Length() + 40);
        prompt->Des().Copy(T(_L("Cloud password")));
        if (hint->Length()) {
            prompt->Des().Append(T(_L(" (hint: ")));
            prompt->Des().Append(*hint);
            prompt->Des().Append(')');
        }
        TTgJob &j = iWorker->Job();
        TBool ok = QueryUtf8L(*prompt, j.iText, sizeof j.iText, R_TG_PASSWORD_QUERY);
        CleanupStack::PopAndDestroy(2);
        if (!ok) {
            ShowLoggedOutL();
            return;
        }
        ShowStatusL(T(_L("Checking password...\nThis takes a few seconds")));
        StartJobL(EJobPassword);
    }

    void StartQrLoginL()
    {
        iLoggedOut = ETrue;
        iQrActive = ETrue;
        ShowStatusL(T(_L("Getting a QR code...")));
        SetCbaL(R_AVKON_SOFTKEYS_OPTIONS_BACK);
        StartJobL(EJobQrLogin);
    }

    void ShowQrL(const char *aUrl)
    {
        if (iScreen != EScreenQr) {
            CloseMsgView();
            HideTabs();
            HideNaviLabel();
            Retire(iImage);
            iImage = NULL;
            Retire(iList);
            iList = NULL;
            iQr = CTgQr::NewL(ClientRect());
            AddToStackL(iQr);
            iScreen = EScreenQr;
            iTitle = T(_L("Scan to log in"));
            SetCbaL(R_AVKON_SOFTKEYS_OPTIONS_BACK);
        }
        SetTitleL(iTitle);
        iQr->SetTextL(aUrl);
    }

    void StopQr()
    {
        iQrActive = EFalse;
        iQrTimer->Cancel();
    }

    static TInt QrTick(TAny *aSelf)
    {
        CTgAppUi *self = static_cast<CTgAppUi *>(aSelf);
        self->iQrTimer->Cancel();
        if (self->iQrActive && !self->iWorker->Busy()) {
            self->iWorker->Job().iType = EJobQrLogin;
            self->iWorker->Start();
        }
        return 0;
    }

    void StartLoginL()
    {
        iLoggedOut = ETrue;
        char phone[32];
        if (!QueryUtf8L(T(_L("Your phone number, e.g. +380...")), phone, sizeof phone,
                        R_TG_NUMBER_QUERY)) {
            ShowLoggedOutL();
            return;
        }
        strcpy(iWorker->Job().iText, phone);
        ShowStatusL(T(_L("Requesting a login code...")));
        StartJobL(EJobSendCode);
    }

    void AskCodeL()
    {
        char code[16];
        TBuf<160> prompt;
        if (!strncmp(iCodeInfo, "auth.sentCodeTypeApp", 20))
            prompt = T(_L("Code sent to your other Telegram app"));
        else if (!strncmp(iCodeInfo, "auth.sentCodeTypeSms", 20))
            prompt = T(_L("Code sent by SMS"));
        else if (!strncmp(iCodeInfo, "auth.sentCodeTypeSetUpEmailRequired", 35))
            prompt = T(_L("Set up a login email in the official app first. Code"));
        else
            prompt = T(_L("Login code"));
        HBufC *info = TextLC(iCodeInfo);
        HBufC *status = HBufC::NewLC(info->Length() + 80);
        status->Des().Copy(T(_L("Waiting for the code\nTelegram says: ")));
        status->Des().Append(*info);
        ShowStatusL(*status);
        CleanupStack::PopAndDestroy(2);
        if (!QueryUtf8L(prompt, code, sizeof code, R_TG_NUMBER_QUERY)) {
            if (AskYesNoL(T(_L("No code? Ask Telegram to send it again?")))) {
                ShowStatusL(T(_L("Requesting the code again...")));
                StartJobL(EJobResendCode);
            } else {
                ShowLoggedOutL();
            }
            return;
        }
        strcpy(iWorker->Job().iText, code);
        ShowStatusL(T(_L("Signing in...")));
        StartJobL(EJobSignIn);
    }

    void AddChatRowL(CTgList *aList, const tg_dialog &d)
    {
        HBufC *name = TextLC(d.name);
        HBufC *text = TextLC(d.text);
        HBufC *title = HBufC::NewLC(name->Length() + 16);
        TPtr tp = title->Des();
        tp.Copy(*name);
        HBufC *sub = HBufC::NewLC(text->Length() + 8);
        TPtr sp = sub->Des();
        if (d.out)
            sp.Copy(T(_L("You: ")));
        sp.Append(*text);
        TInt found = d.photo.type == TG_LOC_PEER_PHOTO ? FindAvatar(d.photo.id) : KErrNotFound;
        TInt icon = aList->AddIconL(d.flags & TG_DLG_SELF ? SavedIconL(aList->IconSize())
                                    : found >= 0 ? AvatarIconL(found)
                                    : LetterIconL(aList->IconSize(), *name, d.id));
        iRowIcon.AppendL(icon);
        TBool muted = (d.flags & TG_DLG_MUTED) != 0;
        TInt mute = muted ? aList->AddIconL(MuteIconL(aList->SubIconSize(3)))
                    : d.flags & TG_DLG_PINNED ? aList->AddIconL(PinIconL(aList->SubIconSize(3)))
                                              : -1;
        TInt badge = d.unread || (d.flags & TG_DLG_UNREAD_MARK)
                         ? aList->AddIconL(BadgeIconL(aList->SubIconSize(4), d.unread, muted))
                         : -1;
        if (badge < 0 && d.out && !(d.flags & TG_DLG_SELF))
            badge = aList->AddIconL(CheckIconL(aList->SubIconSize(4), d.top_id <= d.read_out_max));
        aList->AddRowL(icon, *title, *sub, mute, badge);
        CleanupStack::PopAndDestroy(4);
    }

    void ShowChatsL()
    {
        TInt i, j;
        CTgList *list = ListL(EScreenChats, ETrue);
        iRows.Reset();
        iRowIcon.Reset();
        TBool archive = iTab == ArchiveTab(), all = !iNumFolders || iFolders[iTab].all;
        if (!archive)
            for (i = 0; i < iNumDialogs; i++)
                if (InTab(iDialogs[i]))
                    iRows.AppendL(i);
        if (archive || !all)
            for (i = 0; i < iNumArchive; i++)
                if (InTab(iArchive[i]))
                    iRows.AppendL(iNumDialogs + i);
        if (!archive && !all)
            for (i = 1; i < iRows.Count(); i++)
                for (j = i; j > 0 && DialogAt(iRows[j])->date > DialogAt(iRows[j - 1])->date; j--) {
                    TInt t = iRows[j];
                    iRows[j] = iRows[j - 1];
                    iRows[j - 1] = t;
                }
        for (i = 0; i < iRows.Count(); i++)
            AddChatRowL(list, *DialogAt(iRows[i]));
        if (archive && !iArchiveLoaded)
            list->SetEmptyTextL(T(_L("Loading...")));
        else
            list->SetEmptyTextL(T(_L("No chats")));
        TInt sel = 0;
        for (i = 0; i < iRows.Count(); i++)
            if (DialogAt(iRows[i])->id == iSelId && DialogAt(iRows[i])->kind == iSelKind)
                sel = i;
        list->DoneL(sel);
        UpdateTabBadgesL();
        ShowTabsL();
        RequestAvatarsL();
        iTitle = _L("Telegram");
        SetTitleL(iTitle);
        SetCbaL(R_AVKON_SOFTKEYS_OPTIONS_EXIT);
    }

    TBool CurrentChat(tg_dialog *&aDialog)
    {
        if (!iList || iScreen != EScreenChats)
            return EFalse;
        TInt row = iList->CurrentIndex();
        if (row < 0 || row >= iRows.Count())
            return EFalse;
        aDialog = DialogAt(iRows[row]);
        return aDialog != NULL;
    }

    void OpenChatL()
    {
        tg_dialog *d;
        if (CurrentChat(d))
            OpenChatWithL(d);
    }

    void OpenChatWithL(tg_dialog *d)
    {
        iSelId = d->id;
        iSelKind = d->kind;
        memset(&iCur, 0, sizeof iCur);
        iCur.kind = d->kind;
        iCur.id = d->id;
        iCur.access_hash = d->access_hash;
        iCur.flags = d->flags;
        iCur.unread = d->unread;
        iCur.read_max = d->read_max;
        iCur.read_out_max = d->read_out_max;
        iCur.photo = d->photo;
        iReadUpTo = d->read_max;
        iFirstShow = ETrue;
        d->unread = 0;
        HBufC *name = TextLC(d->name);
        iChatName = name->Left(iChatName.MaxLength());
        CleanupStack::PopAndDestroy(name);
        tg_messages_free(iMessages, iNumMessages);
        iMessages = NULL;
        iNumMessages = 0;
        iAtBottom = ETrue;
        HideTabs();
        delete iPinned;
        iPinned = NULL;
        iStatus = TG_STATUS_NONE;
        UpdateThreadNaviL();
        CTgList *list = ListL(EScreenThread, EFalse);
        list->SetEmptyTextL(T(_L("Loading...")));
        list->DoneL(-1);
        iTitle = iChatName;
        SetTitleL(iTitle);
        SetCbaL(R_AVKON_SOFTKEYS_OPTIONS_BACK);
        LoadThreadL(EFalse);
    }

    void LoadThreadL(TBool aQuiet)
    {
        if (aQuiet && iWorker->Busy())
            return;
        TTgJob &j = iWorker->Job();
        j.iPeer = iCur;
        j.iOffsetId = 0;
        j.iMarkRead = iForeground;
        j.iReadAbove = iReadUpTo;
        StartJobL(EJobHistory, aQuiet);
    }

    TInt32 CurrentMessageId() const
    {
        TInt row = iScreen == EScreenThread && iList ? iList->CurrentIndex()
                   : iScreen == EScreenImage || iScreen == EScreenCamera ? iViewIndex : -1;
        if (row < 0 || row >= iMsgRows.Count() || iMsgRows[row] < 0 ||
            iMsgRows[row] >= iNumMessages)
            return -1;
        return iMessages[iMsgRows[row]].id;
    }

    void MergeMessagesL(tg_message *aNew, TInt aCount)
    {
        if (!aCount) {
            free(aNew);
            return;
        }
        TInt32 lo = aNew[0].id, hi = aNew[aCount - 1].id;
        TInt before = 0, after = 0, gone = 0, i, k = 0;
        for (i = 0; i < iNumMessages; i++) {
            if (iMessages[i].id < lo)
                before++;
            else if (iMessages[i].id > hi)
                after++;
            else
                gone++;
        }
        tg_message *res = (tg_message *)calloc(before + aCount + after, sizeof *res);
        tg_message *dead = (tg_message *)calloc(gone ? gone : 1, sizeof *dead);
        if (!res || !dead) {
            free(res);
            free(dead);
            tg_messages_free(aNew, aCount);
            User::Leave(KErrNoMemory);
        }
        TInt d = 0;
        for (i = 0; i < iNumMessages; i++)
            if (iMessages[i].id < lo)
                res[k++] = iMessages[i];
        for (i = 0; i < aCount; i++)
            res[k++] = aNew[i];
        for (i = 0; i < iNumMessages; i++) {
            if (iMessages[i].id > hi)
                res[k++] = iMessages[i];
            else if (iMessages[i].id >= lo)
                dead[d++] = iMessages[i];
        }
        tg_messages_free(dead, gone);
        free(iMessages);
        free(aNew);
        iMessages = res;
        iNumMessages = k;
    }

    void ShowThreadL(TInt32 aKeepId = -2)
    {
        TInt i;
        TInt32 keepId = aKeepId == -2 ? CurrentMessageId() : aKeepId;
        if (iList && iScreen == EScreenThread && iList->Count())
            iAtBottom = iList->CurrentIndex() >= iList->Count() - 1;
        else if (iScreen == EScreenImage || iScreen == EScreenCamera)
            iAtBottom = EFalse;
        if (iOlderJustLoaded)
            iAtBottom = EFalse;
        CTgList *list = ListL(EScreenThread, ETrue);
        iThumbSize = list->IconSize();
        iMsgRows.Reset();
        TInt divider = -1, keep = -1;
        TInt sent = list->AddIconL(CheckIconL(list->SubIconSize(3), EFalse));
        TInt seen = list->AddIconL(CheckIconL(list->SubIconSize(3), ETrue));
        RArray<TUint32> emojiCp;
        RArray<TInt> emojiIdx;
        CleanupClosePushL(emojiCp);
        CleanupClosePushL(emojiIdx);
        for (i = 0; i < iNumMessages; i++) {
            TInt albumCount = 1, rep = i;
            if (iMessages[i].grouped_id) {
                while (i + albumCount < iNumMessages &&
                       iMessages[i + albumCount].grouped_id == iMessages[i].grouped_id)
                    albumCount++;
                for (TInt k = i; k < i + albumCount; k++)
                    if (iMessages[k].has_text) {
                        rep = k;
                        break;
                    }
            }
            TInt first = i;
            i += albumCount - 1;
            tg_message &m = iMessages[rep];
            if (divider < 0 && iCur.unread > 0 && !m.out && iMessages[i].id > iCur.read_max)
                divider = iMsgRows.Count();
            if (m.id == keepId)
                keep = iMsgRows.Count();
            TBuf<160> head;
            MessageHeadL(m, head, i == 0 || LocalDay(iMessages[i - 1].date) != LocalDay(m.date));
            HBufC *text = TextLC(m.text);
            if (albumCount > 1) {
                HBufC *plain = PlainTextLC(m);
                HBufC *album = HBufC::NewLC(plain->Length() + 32);
                const tg_message &f = iMessages[first];
                const TDesC &kind = f.media.type == TG_LOC_PHOTO || f.save_kind == TG_SAVE_VIDEO
                                        ? T(_L("Album (%d)"))
                                    : f.save_kind == TG_SAVE_AUDIO ? T(_L("Audio (%d)"))
                                                                   : T(_L("Files (%d)"));
                album->Des().Format(kind, albumCount);
                if (m.has_text) {
                    album->Des().Append(_L(": "));
                    album->Des().Append(*plain);
                }
                CleanupStack::Pop(album);
                CleanupStack::PopAndDestroy(2);
                text = album;
                CleanupStack::PushL(text);
            }
            HBufC *line = text;
            if (m.reply) {
                HBufC *reply = TextLC(m.reply);
                line = HBufC::NewLC(reply->Length() + text->Length() + 4);
                line->Des().Append('[');
                line->Des().Append(*reply);
                line->Des().Append(_L("] "));
                line->Des().Append(*text);
            }
            TInt react = -1;
            TUint32 key = 2166136261u;
            for (TInt r = 0; r < m.react_kinds; r++)
                if (HasEmoji(m.react_cp[r]))
                    key = (key ^ m.react_cp[r]) * 16777619u;
            if (key != 2166136261u) {
                TInt at = emojiCp.Find(key);
                if (at < 0) {
                    CGulIcon *ic = ReactionStripL(m, list->SubIconSize(4).iHeight);
                    if (ic) {
                        emojiCp.AppendL(key);
                        emojiIdx.AppendL(list->AddIconL(ic));
                        at = emojiCp.Count() - 1;
                    }
                }
                if (at >= 0)
                    react = emojiIdx[at];
            }
            TInt pic = -1;
            for (TInt k = first; k <= i && pic < 0; k++) {
                const tg_file_loc &tl = iMessages[k].thumb;
                if (tl.type == TG_LOC_NONE)
                    continue;
                TInt found = FindAvatar(tl.id);
                if (found >= 0) {
                    pic = list->AddIconL(AvatarIconL(found));
                } else if (iAvatarAsked.Find(tl.id) < 0) {
                    iAvatarAsked.AppendL(tl.id);
                    TFileName path;
                    char cpath[128];
                    CachePath('t', tl.id, tl.thumb, path, cpath, sizeof cpath);
                    if (FileExists(path))
                        iDecoder->AddL(path, tl.id, 4, iThumbSize);
                    else
                        iWanted.AppendL(tl);
                }
                break;
            }
            list->AddRowL(pic, head, *line,
                          m.out && !(iCur.flags & TG_DLG_SELF)
                              ? (m.id <= iCur.read_out_max ? seen : sent)
                              : -1,
                          react, m.out);
            CleanupStack::PopAndDestroy(m.reply ? 3 : 1);
            iMsgRows.AppendL(rep);
        }
        CleanupStack::PopAndDestroy(2);
        list->SetEmptyTextL(T(_L("No messages\nPress R to write")));
        TInt rows = iMsgRows.Count();
        if (iFirstShow && divider >= 0) {
            list->DoneL(divider);
            list->SetTopL(divider);
            iAtBottom = EFalse;
        } else if (!iAtBottom && keep >= 0) {
            list->DoneL(keep);
        } else {
            list->DoneL(rows - 1);
        }
        if (iNumMessages)
            iFirstShow = EFalse;
        iOlderJustLoaded = EFalse;
        iTitle = iChatName;
        SetTitleL(iTitle);
        SetCbaL(R_AVKON_SOFTKEYS_OPTIONS_BACK);
    }

    void MessageHeadL(const tg_message &aMsg, TDes &aHead, TBool aWithDay = EFalse)
    {
        aHead.Zero();
        if (aWithDay) {
            DayText(aMsg.date, aHead);
            aHead.Append(_L(", "));
        }
        TBuf<8> t;
        TimeText(aMsg.date, t);
        aHead.Append(t);
        if (aMsg.out) {
            aHead.Append(T(_L(" \x00B7 You")));
        } else if (aMsg.from) {
            HBufC *from = TextLC(aMsg.from);
            aHead.Append(_L(" \x00B7 "));
            aHead.Append(from->Left(Max(0, aHead.MaxLength() - aHead.Length() - 40)));
            CleanupStack::PopAndDestroy(from);
        }
        if (aMsg.fwd) {
            HBufC *fwd = TextLC(aMsg.fwd);
            aHead.Append(T(_L(" \x00B7 Fwd: ")));
            aHead.Append(fwd->Left(Max(0, aHead.MaxLength() - aHead.Length() - 30)));
            CleanupStack::PopAndDestroy(fwd);
        }
        if (aMsg.edited && aHead.Length() + 10 < aHead.MaxLength())
            aHead.Append(T(_L(" \x00B7 edited")));
        TBool pictured = EFalse;
        for (TInt r = 0; r < aMsg.react_kinds && !pictured; r++)
            pictured = HasEmoji(aMsg.react_cp[r]);
        if (!pictured && aMsg.reactions_short) {
            HBufC *r = TextLC(aMsg.reactions_short);
            aHead.Append(_L(" \x00B7 "));
            aHead.Append(r->Left(Max(0, aHead.MaxLength() - aHead.Length())));
            CleanupStack::PopAndDestroy(r);
        }
    }

    tg_message *SelectedMessage()
    {
        TInt row = iScreen == EScreenImage ? iViewIndex : iList ? iList->CurrentIndex() : -1;
        if (row < 0 || row >= iMsgRows.Count() || iMsgRows[row] < 0)
            return NULL;
        return &iMessages[iMsgRows[row]];
    }

    HBufC *PlainTextLC(const tg_message &m)
    {
        HBufC *text = TextLC(m.text);
        TPtr tp = text->Des();
        TInt colon = tp.Find(_L(": "));
        if (m.media.type != TG_LOC_NONE && m.has_text && colon >= 0)
            tp.Delete(0, colon + 2);
        return text;
    }

    void LoadOlderL()
    {
        if (iScreen != EScreenThread || !iNumMessages || iWorker->Busy())
            return;
        TTgJob &j = iWorker->Job();
        j.iPeer = iCur;
        j.iOffsetId = iMessages[0].id;
        j.iMarkRead = EFalse;
        j.iReadAbove = iReadUpTo;
        SetTitleL(T(_L("Loading older...")));
        StartJobL(EJobHistory);
    }

    TPtrC DraftL()
    {
        for (TInt i = 0; i < iDrafts.Count(); i++)
            if (iDrafts[i].iId == iCur.id && iDrafts[i].iKind == iCur.kind)
                return *iDrafts[i].iText;
        return TPtrC(KNullDesC);
    }

    void SetDraftL(const TDesC &aText)
    {
        for (TInt i = 0; i < iDrafts.Count(); i++)
            if (iDrafts[i].iId == iCur.id && iDrafts[i].iKind == iCur.kind) {
                delete iDrafts[i].iText;
                iDrafts.Remove(i);
                break;
            }
        if (!aText.Length())
            return;
        TTgDraft d;
        d.iId = iCur.id;
        d.iKind = iCur.kind;
        d.iText = aText.AllocLC();
        iDrafts.AppendL(d);
        CleanupStack::Pop(d.iText);
    }

    TInt PickReactionL()
    {
        const TInt n = sizeof KEmoji / sizeof KEmoji[0];
        CAknSingleGraphicPopupMenuStyleListBox *lb = new (ELeave) CAknSingleGraphicPopupMenuStyleListBox;
        CleanupStack::PushL(lb);
        CAknPopupList *popup =
            CAknPopupList::NewL(lb, R_AVKON_SOFTKEYS_OK_CANCEL, AknPopupLayouts::EMenuGraphicWindow);
        CleanupStack::PushL(popup);
        lb->ConstructL(popup, EAknListBoxSelectionList | EAknListBoxLoopScrolling);
        lb->CreateScrollBarFrameL(ETrue);
        lb->ScrollBarFrame()->SetScrollBarVisibilityL(CEikScrollBarFrame::EOff,
                                                      CEikScrollBarFrame::EAuto);
        CArrayPtr<CGulIcon> *icons = new (ELeave) CArrayPtrFlat<CGulIcon>(n + 1);
        lb->ItemDrawer()->ColumnData()->SetIconArray(icons);
        TInt h = Max(16, lb->ItemHeight() - 6);
        TSize size(h, h);
        CDesCArrayFlat *items = new (ELeave) CDesCArrayFlat(n + 1);
        CleanupStack::PushL(items);
        for (TInt i = 0; i < n; i++) {
            CGulIcon *ic = EmojiIconL(KEmoji[i].iCp, size);
            if (!ic) {
                CFbsBitmap *blank = new (ELeave) CFbsBitmap;
                CleanupStack::PushL(blank);
                User::LeaveIfError(blank->Create(TSize(1, 1), EColor64K));
                ic = CGulIcon::NewL(blank);
                CleanupStack::Pop(blank);
            }
            CleanupStack::PushL(ic);
            icons->AppendL(ic);
            CleanupStack::Pop(ic);
            TBuf<48> row;
            row.AppendNum(i);
            row.Append('\t');
            row.Append(T(TPtrC(KEmoji[i].iName)));
            items->AppendL(row);
        }
        TBuf<48> rm;
        rm.Append(_L("\t"));
        rm.Append(T(_L("Remove my reaction")));
        items->AppendL(rm);
        lb->Model()->SetItemTextArray(items);
        lb->Model()->SetOwnershipType(ELbmDoesNotOwnItemArray);
        popup->SetTitleL(T(_L("React")));
        CleanupStack::Pop(items);
        CleanupStack::Pop(popup);
        CleanupStack::PushL(items);
        TInt index = -1;
        if (popup->ExecuteLD())
            index = lb->CurrentItemIndex();
        CleanupStack::PopAndDestroy(2);
        if (index < 0)
            return -1;
        return index >= n ? -2 : index;
    }

    void CopyTextL(const TDesC &aText)
    {
        CClipboard *cb = CClipboard::NewForWritingLC(iEikonEnv->FsSession());
        CPlainText *pt = CPlainText::NewL();
        CleanupStack::PushL(pt);
        pt->InsertL(0, aText);
        pt->CopyToStoreL(cb->Store(), cb->StreamDictionary(), 0, pt->DocumentLength());
        cb->CommitL();
        CleanupStack::PopAndDestroy(2);
        CAknConfirmationNote *note = new (ELeave) CAknConfirmationNote;
        note->ExecuteLD(T(_L("Copied")));
    }

    TBool HasLinkL(const tg_message &aMsg)
    {
        CDesCArrayFlat *links = new (ELeave) CDesCArrayFlat(2);
        CleanupStack::PushL(links);
        HBufC *t = PlainTextLC(aMsg);
        FindLinksL(*t, *links);
        TBool any = links->Count() > 0;
        CleanupStack::PopAndDestroy(2);
        return any;
    }

    void OpenLinkL(const tg_message &aMsg)
    {
        CDesCArrayFlat *links = new (ELeave) CDesCArrayFlat(2);
        CleanupStack::PushL(links);
        HBufC *t = PlainTextLC(aMsg);
        FindLinksL(*t, *links);
        CleanupStack::PopAndDestroy(t);
        TInt pick = links->Count() == 1 ? 0 : links->Count() ? PickL(T(_L("Choose a link")), links) : -1;
        if (pick >= 0)
            OpenUrlL((*links)[pick]);
        CleanupStack::PopAndDestroy(links);
    }

    static TBool IsFile(const tg_message &aMsg)
    {
        return aMsg.full.type == TG_LOC_DOCUMENT && aMsg.save_kind != TG_SAVE_IMAGE;
    }

    TBool CanEdit(const tg_message &aMsg) const
    {
        return aMsg.out && !aMsg.fwd && !(iCur.flags & TG_DLG_READONLY);
    }

    void ShowShortcutsL()
    {
        static const struct {
            const TText *iKey;
            const TText *iWhat;
        } KChat[] = {{_S("R"), _S("New message")}, {_S("Q"), _S("Reply to")},
                     {_S("E"), _S("Edit")}, {_S("Backspace"), _S("Delete")},
                     {_S("F"), _S("Forward")}, {_S("L"), _S("React")},
                     {_S("C"), _S("Copy text")}, {_S("O"), _S("Open link")},
                     {_S("S"), _S("Save to memory card")}, {_S("P"), _S("Pinned message")},
                     {_S("I"), _S("Info")}, {_S("G"), _S("Send photos")},
                     {_S("K"), _S("Camera")}, {_S("T / B"), _S("First / last message")}},
          KList[] = {{_S("M"), _S("Mute / unmute")}, {_S("P"), _S("Pin / unpin")},
                     {_S("A"), _S("Archive / unarchive")}, {_S("U"), _S("Read / unread")},
                     {_S("I"), _S("Info")}, {_S("C"), _S("Contacts")},
                     {_S("F"), _S("Find by username")}, {_S("< / >"), _S("Folders")}};
        HBufC *text = HBufC::NewLC(2000);
        TPtr p = text->Des();
        p.Append(T(_L("In a chat")));
        p.Append('\n');
        for (TUint i = 0; i < sizeof KChat / sizeof KChat[0]; i++) {
            p.Append(TPtrC(KChat[i].iKey));
            p.Append(_L(" \x2014 "));
            p.Append(T(TPtrC(KChat[i].iWhat)));
            p.Append('\n');
        }
        p.Append('\n');
        p.Append(T(_L("In the chat list")));
        p.Append('\n');
        for (TUint i = 0; i < sizeof KList / sizeof KList[0]; i++) {
            p.Append(TPtrC(KList[i].iKey));
            p.Append(_L(" \x2014 "));
            p.Append(T(TPtrC(KList[i].iWhat)));
            p.Append('\n');
        }
        CAknMessageQueryDialog *dlg = CAknMessageQueryDialog::NewL(*text);
        dlg->PrepareLC(R_AVKON_MESSAGE_QUERY_DIALOG);
        if (dlg->QueryHeading())
            dlg->QueryHeading()->SetTextL(T(_L("Shortcuts")));
        dlg->RunLD();
        CleanupStack::PopAndDestroy(text);
    }

    void DeleteMessageL(const tg_message &aMsg)
    {
        TTgJob &j = iWorker->Job();
        j.iNumIds = 0;
        for (TInt i = 0; i < iNumMessages && j.iNumIds < 10; i++)
            if (iMessages[i].id == aMsg.id ||
                (aMsg.grouped_id && iMessages[i].grouped_id == aMsg.grouped_id))
                j.iIds[j.iNumIds++] = iMessages[i].id;
        j.iRevoke = 1;
        if (iCur.kind != 2 && !(iCur.flags & TG_DLG_SELF) && (aMsg.out || iCur.kind == 0)) {
            CDesCArrayFlat *opts = new (ELeave) CDesCArrayFlat(2);
            CleanupStack::PushL(opts);
            opts->AppendL(T(_L("Delete for everyone")));
            opts->AppendL(T(_L("Delete for me")));
            TInt pick = PickL(T(_L("Delete")), opts);
            CleanupStack::PopAndDestroy(opts);
            if (pick < 0)
                return;
            j.iRevoke = pick == 0;
        } else if (!AskYesNoL(T(_L("Delete this message?")))) {
            return;
        }
        j.iPeer = iCur;
        StartJobL(EJobDelete);
    }

    void DropMessages(const int32_t *aIds, TInt aCount)
    {
        tg_message *dead = (tg_message *)calloc(aCount, sizeof *dead);
        if (!dead)
            return;
        TInt k = 0, d = 0;
        for (TInt i = 0; i < iNumMessages; i++) {
            TBool drop = EFalse;
            for (TInt n = 0; n < aCount && !drop; n++)
                drop = iMessages[i].id == aIds[n];
            if (drop && d < aCount)
                dead[d++] = iMessages[i];
            else
                iMessages[k++] = iMessages[i];
        }
        iNumMessages = k;
        tg_messages_free(dead, d);
    }

    void MessageActionL(TInt aCommand)
    {
        tg_message *m = SelectedMessage();
        if (!m || iScreen != EScreenThread)
            return;
        TTgJob &j = iWorker->Job();
        if (aCommand == ETgCmdReplyTo) {
            ReplyL(m->id);
        } else if (aCommand == ETgCmdOpenLink) {
            OpenLinkL(*m);
        } else if (aCommand == ETgCmdCopy) {
            HBufC *t = PlainTextLC(*m);
            CopyTextL(*t);
            CleanupStack::PopAndDestroy(t);
        } else if (aCommand == ETgCmdEdit) {
            HBufC *t = PlainTextLC(*m);
            if (!m->has_text)
                t->Des().Zero();
            else if (m->full.type != TG_LOC_NONE && m->media.type == TG_LOC_NONE &&
                     t->Find(_L(": ")) >= 0)
                t->Des().Delete(0, t->Find(_L(": ")) + 2);
            TBool ok = QueryUtf8L(T(_L("Edit message")), j.iText, sizeof j.iText, R_TG_PROSE_QUERY, *t);
            CleanupStack::PopAndDestroy(t);
            if (ok) {
                j.iPeer = iCur;
                j.iMsgId = m->id;
                StartJobL(EJobEdit);
            }
        } else if (aCommand == ETgCmdDelete) {
            DeleteMessageL(*m);
        } else if (aCommand == ETgCmdForward) {
            CDesCArrayFlat *names = new (ELeave) CDesCArrayFlat(32);
            CleanupStack::PushL(names);
            for (TInt i = 0; i < iNumDialogs + iNumArchive; i++) {
                HBufC *n = TextLC(DialogAt(i)->name);
                names->AppendL(*n);
                CleanupStack::PopAndDestroy(n);
            }
            TInt32 id = m->id;
            TInt pick = PickL(T(_L("Forward to")), names);
            CleanupStack::PopAndDestroy(names);
            if (pick >= 0) {
                j.iPeer = iCur;
                j.iMsgId = id;
                j.iTarget = *DialogAt(pick);
                StartJobL(EJobForward);
            }
        } else if (aCommand == ETgCmdReact) {
            TInt32 id = m->id;
            TInt pick = PickReactionL();
            if (pick == -1)
                return;
            j.iText[0] = 0;
            if (pick >= 0) {
                TUint32 cp = KEmoji[pick].iCp;
                TPtr8 u((TUint8 *)j.iText, 0, 8);
                if (cp < 0x800) {
                    u.Append(TUint8(0xc0 | (cp >> 6)));
                    u.Append(TUint8(0x80 | (cp & 63)));
                } else if (cp < 0x10000) {
                    u.Append(TUint8(0xe0 | (cp >> 12)));
                    u.Append(TUint8(0x80 | ((cp >> 6) & 63)));
                    u.Append(TUint8(0x80 | (cp & 63)));
                } else {
                    u.Append(TUint8(0xf0 | (cp >> 18)));
                    u.Append(TUint8(0x80 | ((cp >> 12) & 63)));
                    u.Append(TUint8(0x80 | ((cp >> 6) & 63)));
                    u.Append(TUint8(0x80 | (cp & 63)));
                }
                j.iText[u.Length()] = 0;
            }
            j.iPeer = iCur;
            j.iMsgId = id;
            StartJobL(EJobReact);
        }
    }

    void ShowMessageL(TBool aFromImage)
    {
        TInt row = aFromImage ? iViewIndex : iList ? iList->CurrentIndex() : -1;
        if (iMsgView || row < 0 || row >= iMsgRows.Count() || iMsgRows[row] < 0)
            return;
        tg_message &m = iMessages[iMsgRows[row]];
        TBool media = !aFromImage && m.media.type != TG_LOC_NONE;
        if (!aFromImage)
            SetAlbumFor(m);
        if (!aFromImage && !media && iAlbum.Count()) {
            media = ETrue;
        }
        if (!aFromImage && IsFile(m) && !m.has_text) {
            SaveMediaL();
            return;
        }
        if (media && !m.has_text) {
            OpenMediaL(iAlbum.Count() ? iMessages[iAlbum[iAlbumPos]] : m);
            return;
        }
        HBufC *plain = PlainTextLC(m);
        TInt extra = 64 + (m.fwd ? User::StringLength((const TUint8 *)m.fwd) : 0) +
                     (m.reply ? User::StringLength((const TUint8 *)m.reply) : 0);
        HBufC *text = HBufC::NewLC(plain->Length() + extra);
        TPtr tp = text->Des();
        if (m.fwd) {
            HBufC *f = TextLC(m.fwd);
            tp.Append(T(_L("Forwarded from ")));
            tp.Append(*f);
            tp.Append(_L("\n\n"));
            CleanupStack::PopAndDestroy(f);
        }
        if (m.reply) {
            HBufC *r = TextLC(m.reply);
            tp.Append(T(_L("In reply to ")));
            tp.Append(*r);
            tp.Append(_L("\n\n"));
            CleanupStack::PopAndDestroy(r);
        }
        tp.Append(*plain);
        CleanupStack::Pop(text);
        CleanupStack::PopAndDestroy(plain);
        CleanupStack::PushL(text);
        RArray<TTgChip> chips;
        CleanupClosePushL(chips);
        TInt side = CTgMsgView::IconSide();
        for (TInt i = 0; i < m.react_kinds; i++) {
            TTgChip c;
            c.iIcon = m.react_cp[i] ? EmojiIconL(m.react_cp[i], TSize(side, side)) : NULL;
            c.iCount = m.react_count[i];
            HBufC *t = TextLC(m.react_text[i]);
            c.iText = t->Left(8);
            if (!c.iText.Length() && !c.iIcon)
                c.iText = _L("*");
            if (!c.iIcon)
                c.iText.Append(' ');
            CleanupStack::PopAndDestroy(t);
            if (chips.Append(c) != KErrNone)
                delete c.iIcon;
        }
        TBuf<160> head;
        MessageHeadL(m, head, ETrue);
        CleanupStack::Pop(&chips);
        CleanupStack::Pop(text);
        CTgMsgView *view = NULL;
        TRAPD(err, view = CTgMsgView::NewL(ClientRect(), *this, head, text, chips));
        for (TInt i = 0; i < chips.Count(); i++)
            delete chips[i].iIcon;
        chips.Close();
        User::LeaveIfError(err);
        iMsgView = view;
        AddToStackL(iMsgView, ECoeStackPriorityDialog);
        iMsgViewMedia = media;
        iMsgViewRow = row;
        iMsgViewFile = !aFromImage && IsFile(m);
        iMsgViewLink = !media && !iMsgViewFile && HasLinkL(m);
        CEikButtonGroupContainer *cba = Cba();
        cba->SetCommandSetL(iMsgViewFile || iMsgViewLink ? R_TG_SOFTKEYS_PREVIEW_BACK
                            : !media                      ? R_TG_SOFTKEYS_REPLY_BACK
                            : m.media.type == TG_LOC_PHOTO ? R_TG_SOFTKEYS_PHOTO_BACK
                                                          : R_TG_SOFTKEYS_PREVIEW_BACK);
        cba->SetCommandL(EAknSoftkeyOk, T(iMsgViewFile ? _L("Download")
                                          : iMsgViewLink ? _L("Open link")
                                          : !media       ? _L("Reply")
                                          : m.media.type == TG_LOC_PHOTO ? _L("Photo")
                                                                         : _L("Preview")));
        cba->SetCommandL(EAknSoftkeyCancel, T(_L("Back")));
        if (!media && !iMsgViewFile && !iMsgViewLink && (iCur.flags & TG_DLG_READONLY))
            cba->SetCommandL(EAknSoftkeyOk, KNullDesC);
        cba->DrawDeferred();
    }

    void CloseMsgView()
    {
        if (!iMsgView)
            return;
        CTgMsgView *v = iMsgView;
        iMsgView = NULL;
        Retire(v);
        TRAP_IGNORE(SetCbaL(R_AVKON_SOFTKEYS_OPTIONS_BACK));
    }

    void MsgViewReplyL()
    {
        CloseMsgView();
        ReplyL();
    }

    void MsgViewOkL()
    {
        TBool media = iMsgViewMedia;
        TInt row = iMsgViewRow;
        CloseMsgView();
        if (iMsgViewFile)
            SaveMediaL();
        else if (iMsgViewLink && row >= 0 && row < iMsgRows.Count() && iMsgRows[row] >= 0)
            OpenLinkL(iMessages[iMsgRows[row]]);
        else if (media && iAlbum.Count())
            OpenMediaL(iMessages[iAlbum[iAlbumPos]]);
        else if (media && row >= 0 && row < iMsgRows.Count() && iMsgRows[row] >= 0)
            OpenMediaL(iMessages[iMsgRows[row]]);
        else if (!media)
            ReplyL();
    }

    void ReplyL(TInt32 aReplyTo = 0)
    {
        if ((iScreen != EScreenThread && iScreen != EScreenImage) || (iCur.flags & TG_DLG_READONLY))
            return;
        TTgJob &j = iWorker->Job();
        char *text = (char *)User::Alloc(sizeof j.iText);
        if (!text)
            return;
        HBufC *cancelled = HBufC::NewLC(1000);
        TPtr cp = cancelled->Des();
        TBool ok = QueryUtf8L(aReplyTo ? T(_L("Reply")) : T(_L("Message")), text, sizeof j.iText,
                              R_TG_PROSE_QUERY, DraftL(), &cp);
        if (ok) {
            SetDraftL(KNullDesC);
            strcpy(j.iText, text);
            j.iPeer = iCur;
            j.iReplyTo = aReplyTo;
            iAtBottom = ETrue;
            StartJobL(EJobSend);
        } else {
            SetDraftL(cp);
        }
        CleanupStack::PopAndDestroy(cancelled);
        User::Free(text);
    }

    static void CountKinds(const tg_dialog *aD, TInt aN, TDes &aOut)
    {
        TInt u = 0, g = 0, c = 0;
        for (TInt i = 0; i < aN; i++) {
            if (aD[i].flags & TG_DLG_GROUP)
                g++;
            else if (aD[i].flags & TG_DLG_CHANNEL)
                c++;
            else
                u++;
        }
        aOut.AppendFormat(_L("%d (people %d, groups %d, channels %d)\n"), aN, u, g, c);
    }

    void ShowDiagnosticsL()
    {
        HBufC *text = HBufC::NewLC(2000);
        TPtr p = text->Des();
        p.Append(_L("Main: "));
        CountKinds(iDialogs, iNumDialogs, p);
        p.Append(_L("Archive: "));
        if (iArchiveLoaded)
            CountKinds(iArchive, iNumArchive, p);
        else
            p.Append(_L("not loaded\n"));
        p.AppendFormat(_L("Tab %d of %d, folders %d\n"), iTab, ArchiveTab() + 1, iNumFolders);
        for (TInt i = 0; i < iNumFolders && p.Length() < 1800; i++) {
            const tg_folder &f = iFolders[i];
            TInt inc = 0, exc = 0, match = 0, j;
            for (j = 0; j < f.npeers; j++)
                f.peers[j].include ? inc++ : exc++;
            for (j = 0; j < iNumDialogs + iNumArchive; j++)
                match += tg_folder_has(&f, DialogAt(j));
            HBufC *t = TextLC(f.title);
            p.AppendFormat(_L("%d. "), i);
            p.Append(t->Left(20));
            p.AppendFormat(_L(": all %d list %d flags %x +%d -%d = %d\n"), f.all, f.chatlist,
                           f.flags, inc, exc, match);
            CleanupStack::PopAndDestroy(t);
        }
        CAknMessageQueryDialog *dlg = CAknMessageQueryDialog::NewL(p);
        dlg->PrepareLC(R_AVKON_MESSAGE_QUERY_DIALOG);
        if (dlg->QueryHeading())
            dlg->QueryHeading()->SetTextL(T(_L("Diagnostics")));
        dlg->RunLD();
        CleanupStack::PopAndDestroy(text);
    }

    void HandleCommandL(TInt aCommand)
    {
        if (iMsgView) {
            if (aCommand == EAknSoftkeyOk)
                MsgViewOkL();
            else if (aCommand == EAknSoftkeyCancel || aCommand == EAknSoftkeyBack)
                CloseMsgView();
            else if (aCommand == EEikCmdExit)
                Exit();
            return;
        }
        switch (aCommand) {
        case EAknSoftkeyBack:
            if (iQrActive) {
                StopQr();
                ShowLoggedOutL();
            } else if (iScreen == EScreenImage || iScreen == EScreenCamera) {
                ShowThreadL();
            } else if (iScreen == EScreenThread) {
                ShowChatsL();
                iChatsDirty = ETrue;
                StartDialogsL(ETrue, EFalse);
            } else {
                Exit();
            }
            break;
        case ETgCmdOpen:
            SelectL();
            break;
        case ETgCmdReply:
            ReplyL();
            break;
        case ETgCmdRefresh:
            if (iScreen == EScreenThread)
                LoadThreadL(EFalse);
            else
                StartDialogsL(EFalse, ETrue);
            break;
        case ETgCmdLogin:
            StopQr();
            StartLoginL();
            break;
        case ETgCmdLoginQr:
            StartQrLoginL();
            break;
        case ETgCmdLogout:
            if (!iWorker->Busy() && AskYesNoL(T(_L("Log out of this account?")))) {
                ShowStatusL(T(_L("Logging out...")));
                StartJobL(EJobLogout);
            }
            break;
        case ETgCmdDebug:
            ShowDiagnosticsL();
            break;
        case ETgCmdLanguage:
            ChooseLanguageL();
            break;
        case ETgCmdText:
            ShowMessageL(ETrue);
            break;
        case ETgCmdZoomIn:
            ZoomL(iImage && iImage->Zoom() >= 2 ? 2 : 1);
            break;
        case ETgCmdZoomOut:
            ZoomL(iImage && iImage->Zoom() >= 4 ? -2 : -1);
            break;
        case ETgCmdSave:
            SaveMediaL();
            break;
        case ETgCmdSendPhoto:
            SendPhotosL();
            break;
        case ETgCmdSendFile:
            SendPickedL(EFalse);
            break;
        case ETgCmdCamera:
            OpenCameraL();
            break;
        case ETgCmdPlay:
            SaveMediaL(ETrue);
            break;
        case ETgCmdReplyTo:
        case ETgCmdOpenLink:
        case ETgCmdCopy:
        case ETgCmdEdit:
        case ETgCmdDelete:
        case ETgCmdForward:
        case ETgCmdReact:
            MessageActionL(aCommand);
            break;
        case ETgCmdOlder:
            LoadOlderL();
            break;
        case ETgCmdInfo:
            if (iScreen == EScreenThread) {
                iWorker->Job().iPeer = iCur;
                StartJobL(EJobInfo);
            }
            break;
        case ETgCmdContacts:
            StartJobL(EJobContacts);
            break;
        case ETgCmdFindUser: {
            TTgJob &j = iWorker->Job();
            if (QueryUtf8L(T(_L("Username, e.g. @durov")), j.iText, sizeof j.iText, R_TG_PROSE_QUERY))
                StartJobL(EJobResolve);
            break;
        }
        case ETgCmdPin:
        case ETgCmdUnpin:
        case ETgCmdMute:
        case ETgCmdUnmute:
        case ETgCmdMarkRead:
        case ETgCmdMarkUnread:
        case ETgCmdArchive:
        case ETgCmdUnarchive: {
            static const TInt actions[] = {TG_ACT_PIN, TG_ACT_UNPIN, TG_ACT_MUTE, TG_ACT_UNMUTE,
                                           TG_ACT_READ, TG_ACT_UNREAD, TG_ACT_ARCHIVE,
                                           TG_ACT_UNARCHIVE};
            tg_dialog *d;
            if (CurrentChat(d)) {
                TTgJob &j = iWorker->Job();
                j.iPeer = *d;
                j.iMsgId = actions[aCommand - ETgCmdPin];
                StartJobL(EJobChatAction);
            }
            break;
        }
        case ETgCmdShortcuts:
            ShowShortcutsL();
            break;
        case ETgCmdPinned:
            if (iPinned) {
                CAknMessageQueryDialog *dlg = CAknMessageQueryDialog::NewL(*iPinned);
                dlg->PrepareLC(R_AVKON_MESSAGE_QUERY_DIALOG);
                if (dlg->QueryHeading())
                    dlg->QueryHeading()->SetTextL(T(_L("Pinned message")));
                dlg->RunLD();
            }
            break;
        case EAknSoftkeyOk:
            if (iScreen == EScreenCamera && iCameraCtl)
                iCameraCtl->Capture();
            break;
        case EAknSoftkeyExit: {
            TApaTask task(iEikonEnv->WsSession());
            task.SetWgId(iEikonEnv->RootWin().Identifier());
            task.SendToBackground();
            break;
        }
        case ETgCmdExit:
        case EEikCmdExit:
            Exit();
            break;
        }
    }

    void DynInitMenuPaneL(TInt aResourceId, CEikMenuPane *aMenu)
    {
        static const struct {
            TInt iCmd;
            const TText *iText;
        } labels[] = {
            {ETgCmdOpen, _S("Open")}, {ETgCmdChat, _S("Chat")}, {ETgCmdReply, _S("Reply")},
            {ETgCmdAttach, _S("Attach")}, {ETgCmdMessage, _S("Message")},
            {ETgCmdPinned, _S("Pinned message")}, {ETgCmdInfo, _S("Info")},
            {ETgCmdContacts, _S("Contacts")}, {ETgCmdFindUser, _S("Find by username")},
            {ETgCmdOlder, _S("Load older messages")}, {ETgCmdText, _S("Message text")},
            {ETgCmdZoomIn, _S("Zoom in")}, {ETgCmdZoomOut, _S("Zoom out")},
            {ETgCmdSave, _S("Save to memory card")}, {ETgCmdRefresh, _S("Refresh")},
            {ETgCmdLoginQr, _S("Log in with QR code")},
            {ETgCmdLogin, _S("Log in with phone number")}, {ETgCmdLogout, _S("Log out")},
            {ETgCmdLanguage, _S("Language")}, {ETgCmdDebug, _S("Diagnostics")},
            {ETgCmdShortcuts, _S("Shortcuts")},
            {ETgCmdExit, _S("Exit")}, {ETgCmdPin, _S("Pin")}, {ETgCmdUnpin, _S("Unpin")},
            {ETgCmdMute, _S("Mute")}, {ETgCmdUnmute, _S("Unmute")},
            {ETgCmdMarkRead, _S("Mark as read")}, {ETgCmdMarkUnread, _S("Mark as unread")},
            {ETgCmdArchive, _S("Archive chat")}, {ETgCmdUnarchive, _S("Unarchive chat")},
            {ETgCmdReplyTo, _S("Reply to")}, {ETgCmdCopy, _S("Copy text")},
            {ETgCmdOpenLink, _S("Open link")},
            {ETgCmdEdit, _S("Edit")}, {ETgCmdDelete, _S("Delete")},
            {ETgCmdForward, _S("Forward")}, {ETgCmdReact, _S("React")},
            {ETgCmdPlay, _S("Open in player")}, {ETgCmdSendPhoto, _S("Photo")},
            {ETgCmdSendFile, _S("File")}, {ETgCmdCamera, _S("Camera")},
            {ETgCmdSendFile, _S("File")}};
        for (TUint li = 0; li < sizeof labels / sizeof labels[0]; li++) {
            TInt pos;
            if (aMenu->MenuItemExists(labels[li].iCmd, pos))
                aMenu->SetItemTextL(labels[li].iCmd, T(TPtrC(labels[li].iText)));
        }
        if (aResourceId == R_TG_CHAT_MENU) {
            tg_dialog *d = NULL;
            CurrentChat(d);
            TBool pinned = d && (d->flags & TG_DLG_PINNED), muted = d && (d->flags & TG_DLG_MUTED);
            TBool unread = d && (d->unread || (d->flags & TG_DLG_UNREAD_MARK));
            TBool archived = d && (d->flags & TG_DLG_ARCHIVED);
            aMenu->SetItemDimmed(ETgCmdPin, !d || pinned);
            aMenu->SetItemDimmed(ETgCmdUnpin, !pinned);
            aMenu->SetItemDimmed(ETgCmdMute, !d || muted);
            aMenu->SetItemDimmed(ETgCmdUnmute, !muted);
            aMenu->SetItemDimmed(ETgCmdMarkRead, !unread);
            aMenu->SetItemDimmed(ETgCmdMarkUnread, !d || unread);
            aMenu->SetItemDimmed(ETgCmdArchive, !d || archived || (d->flags & TG_DLG_SELF));
            aMenu->SetItemDimmed(ETgCmdUnarchive, !archived);
            return;
        }
        if (aResourceId == R_TG_MESSAGE_MENU) {
            tg_message *m = SelectedMessage();
            TBool editable = m && CanEdit(*m);
            aMenu->SetItemDimmed(ETgCmdDelete, !m || (iCur.flags & TG_DLG_READONLY));
            aMenu->SetItemDimmed(ETgCmdEdit, !editable);
            aMenu->SetItemDimmed(ETgCmdCopy, !m || !m->has_text);
            aMenu->SetItemDimmed(ETgCmdReplyTo, !m || (iCur.flags & TG_DLG_READONLY));
            aMenu->SetItemDimmed(ETgCmdOpenLink, !m || !m->has_text || !HasLinkL(*m));
            aMenu->SetItemDimmed(ETgCmdPlay, !m || m->full.type != TG_LOC_DOCUMENT);
            return;
        }
        if (aResourceId != R_TG_MENU)
            return;
        TBool chats = iScreen == EScreenChats;
        TBool thread = iScreen == EScreenThread;
        TBool image = iScreen == EScreenImage;
        tg_message *m = ViewedMessage();
        aMenu->SetItemDimmed(ETgCmdOpen, !(chats || thread));
        TBool readonly = (iCur.flags & TG_DLG_READONLY) != 0;
        aMenu->SetItemDimmed(ETgCmdReply, !(thread || image) || readonly);
        aMenu->SetItemDimmed(ETgCmdText, !image);
        aMenu->SetItemDimmed(ETgCmdAttach, !thread || readonly);
        aMenu->SetItemDimmed(ETgCmdReact, !thread || !SelectedMessage());
        aMenu->SetItemDimmed(ETgCmdLanguage, thread || image || iScreen == EScreenCamera);
        {
            tg_dialog *d = NULL;
            aMenu->SetItemDimmed(ETgCmdChat, !chats || !CurrentChat(d));
        }
        aMenu->SetItemDimmed(ETgCmdMessage, !thread || !SelectedMessage());
        aMenu->SetItemDimmed(ETgCmdPinned, !thread || !iPinned);
        aMenu->SetItemDimmed(ETgCmdOlder, !thread);
        aMenu->SetItemDimmed(ETgCmdInfo, !thread);
        aMenu->SetItemDimmed(ETgCmdContacts, !chats);
        aMenu->SetItemDimmed(ETgCmdFindUser, !chats);
        aMenu->SetItemDimmed(ETgCmdZoomIn, !image || (iImage && iImage->Zoom() >= 4));
        aMenu->SetItemDimmed(ETgCmdZoomOut, !image || (iImage && iImage->Zoom() <= 1));
        aMenu->SetItemDimmed(ETgCmdSave, !m || m->save_kind == TG_SAVE_NONE);
        aMenu->SetItemDimmed(ETgCmdRefresh, !(chats || thread || (iScreen == EScreenStatus && !iLoggedOut)));
        aMenu->SetItemDimmed(ETgCmdLogin, !iLoggedOut);
        aMenu->SetItemDimmed(ETgCmdLoginQr, !iLoggedOut || iQrActive);
        aMenu->SetItemDimmed(ETgCmdLogout, iLoggedOut || !chats);
    }

    void HandleResourceChangeL(TInt aType)
    {
        CAknAppUi::HandleResourceChangeL(aType);
        if (iList)
            iList->SetRect(ClientRect());
        if (iQr)
            iQr->SetRect(ClientRect());
        if (iImage)
            iImage->SetRect(ClientRect());
        if (iCameraCtl)
            iCameraCtl->SetRect(ClientRect());
    }

    CTgWorker *iWorker;
    CAsyncCallBack *iReaper;
    RPointerArray<CCoeControl> iDead;
    CTgList *iList;
    CTgQr *iQr;
    CTgImage *iImage;
    CTgMsgView *iMsgView;
    TBool iMsgViewMedia;
    TInt iMsgViewRow;
    RArray<TInt> iAlbum;
    TInt iAlbumPos;
    CTgCamera *iCameraCtl;
    CAsyncCallBack *iAfterCapture;
    HBufC8 *iCaptured;
    CTgDecoder *iDecoder;
    RArray<TTgAvatar> iAvatars;
    RArray<TTgAvatar> iEmoji;
    RArray<TInt64> iAvatarAsked;
    RArray<tg_file_loc> iWanted;
    TSize iIconSize;
    TSize iThumbSize;
    TBool iFullAsked;
    TFileName iSaveName;
    TInt64 iViewKey;
    TInt iViewIndex;
    CPeriodic *iQrTimer;
    CPeriodic *iPollTimer;
    CPeriodic *iProgressTimer;
    TBool iQrActive;
    TBool iForeground;
    TBool iChatsAfterThread;
    TInt iBgTicks;
    CHWRMVibra *iVibra;
    TBool iInJobDone;
    TBool iQuiet;
    TBool iHasPending;
    TTgJobType iPending;
    TScreen iScreen;
    TBool iLoggedOut;
    TBuf<64> iTitle;
    TBuf<64> iChatName;
    char iPrivDir[64];
    char iCodeInfo[128];
    tg_dialog *iDialogs;
    TInt iNumDialogs;
    tg_dialog *iArchive;
    TInt iNumArchive;
    TBool iArchiveLoaded;
    tg_folder *iFolders;
    TInt iNumFolders;
    TBool iFoldersLoaded;
    CAknNavigationControlContainer *iNavi;
    CAknNavigationDecorator *iTabDeco;
    TBool iTabsShown;
    RArray<TInt> iTabCounts;
    CAknNavigationDecorator *iNaviLabel;
    TInt iTab;
    RArray<TInt> iRows;
    tg_dialog iCur;
    TInt64 iSelId;
    TInt iSelKind;
    tg_message *iMessages;
    TInt iNumMessages;
    TBool iAtBottom;
    TBool iOlderJustLoaded;
    TBool iChatsDirty;
    TBool iOpenAfterSave;
    CDocumentHandler *iDocHandler;
    HBufC *iPinned;
    TInt iStatus;
    TInt iStatusWhen;
    TBool iMsgViewFile;
    TBool iMsgViewLink;
    RArray<TTgDraft> iDrafts;
    TBool iFirstShow;
    TInt iReadUpTo;
    RArray<TInt> iMsgRows;
    RArray<TInt> iRowIcon;
};

CTgMsgView *CTgMsgView::NewL(const TRect &aClient, CTgAppUi &aUi, const TDesC &aHead, HBufC *aBody,
                             RArray<TTgChip> &aChips)
{
    CTgMsgView *self = new (ELeave) CTgMsgView(aUi);
    CleanupStack::PushL(self);
    self->ConstructL(aClient, aHead, aBody, aChips);
    CleanupStack::Pop(self);
    return self;
}

TKeyResponse CTgMsgView::OfferKeyEventL(const TKeyEvent &aKey, TEventCode aType)
{
    if (aType != EEventKey)
        return EKeyWasConsumed;
    if (aKey.iCode == EKeyOK || aKey.iCode == EKeyEnter) {
        iUi.MsgViewOkL();
        return EKeyWasConsumed;
    }
    if (aKey.iCode == EKeyDownArrow) {
        TInt rows = iLines->Count() + ChipRows(iTextW);
        TInt visible = (Rect().Height() - iHeadH - 20) / LineH();
        if (iTop + visible < rows) {
            iTop++;
            DrawDeferred();
        }
        return EKeyWasConsumed;
    }
    if (aKey.iCode == EKeyUpArrow) {
        if (iTop > 0) {
            iTop--;
            DrawDeferred();
        }
        return EKeyWasConsumed;
    }
    if (aKey.iCode == 'r' || aKey.iCode == 'R') {
        iUi.MsgViewReplyL();
        return EKeyWasConsumed;
    }
    return EKeyWasConsumed;
}

CTgCamera *CTgCamera::NewL(const TRect &aRect, CTgAppUi &aUi)
{
    CTgCamera *self = new (ELeave) CTgCamera(aUi);
    CleanupStack::PushL(self);
    self->ConstructL(aRect);
    CleanupStack::Pop(self);
    return self;
}

void CTgCamera::ReserveComplete(TInt aError)
{
    if (aError != KErrNone) {
        TRAP_IGNORE(iUi.CameraFailedL(aError));
    } else {
        iCamera->PowerOn();
    }
}

void CTgCamera::PowerOnComplete(TInt aError)
{
    if (aError != KErrNone) {
        TRAP_IGNORE(iUi.CameraFailedL(aError));
        return;
    }
    TCameraInfo info;
    iCamera->CameraInfo(info);
    CCamera::TFormat fmt = (info.iImageFormatsSupported & CCamera::EFormatExif) ? CCamera::EFormatExif
                                                                                  : CCamera::EFormatJpeg;
    TInt best = -1, bestW = 0, small = 0, smallW = KMaxTInt;
    for (TInt i = 0; i < info.iNumImageSizesSupported; i++) {
        TSize sz;
        iCamera->EnumerateCaptureSizes(sz, i, fmt);
        if (sz.iWidth <= 1600 && sz.iWidth > bestW) {
            best = i;
            bestW = sz.iWidth;
        }
        if (sz.iWidth > 0 && sz.iWidth < smallW) {
            small = i;
            smallW = sz.iWidth;
        }
    }
    TRAPD(err, iCamera->PrepareImageCaptureL(fmt, best >= 0 ? best : small));
    if (err == KErrNone) {
        TSize vf = Rect().Size();
        TRAP(err, iCamera->StartViewFinderBitmapsL(vf));
    }
    if (err != KErrNone) {
        TRAP_IGNORE(iUi.CameraFailedL(err));
        return;
    }
    iReady = ETrue;
}

void CTgCamera::ImageReady(CFbsBitmap *aBitmap, HBufC8 *aData, TInt aError)
{
    delete aBitmap;
    iCapturing = EFalse;
    if (aError != KErrNone || !aData) {
        delete aData;
        TRAP_IGNORE(iUi.CameraFailedL(aError ? aError : KErrNotSupported));
        return;
    }
    TRAP_IGNORE(iUi.PhotoTakenL(aData));
}

TKeyResponse CTgCamera::OfferKeyEventL(const TKeyEvent &aKey, TEventCode aType)
{
    if (aType == EEventKey && (aKey.iCode == EKeyOK || aKey.iCode == EKeyEnter)) {
        Capture();
        return EKeyWasConsumed;
    }
    return EKeyWasNotConsumed;
}

CTgImage *CTgImage::NewL(const TRect &aRect, CTgAppUi &aUi, CFbsBitmap *aBitmap)
{
    CTgImage *self = new (ELeave) CTgImage(aUi, aBitmap);
    CleanupStack::PushL(self);
    self->CreateWindowL();
    self->SetRect(aRect);
    self->ActivateL();
    CleanupStack::Pop(self);
    return self;
}

TKeyResponse CTgImage::OfferKeyEventL(const TKeyEvent &aKey, TEventCode aType)
{
    if (aType == EEventKey) {
        TInt dx = aKey.iCode == EKeyLeftArrow ? -1 : aKey.iCode == EKeyRightArrow ? 1 : 0;
        TInt dy = aKey.iCode == EKeyUpArrow ? -1 : aKey.iCode == EKeyDownArrow ? 1 : 0;
        if ((dx || dy) && Pan(dx, dy))
            return EKeyWasConsumed;
        if (dx) {
            iUi.AlbumStepL(dx);
            return EKeyWasConsumed;
        }
        if (aKey.iCode == EKeyOK || aKey.iCode == EKeyEnter) {
            iUi.SelectL();
            return EKeyWasConsumed;
        }
        if (iUi.ShortcutL(aKey.iCode))
            return EKeyWasConsumed;
    }
    return EKeyWasNotConsumed;
}

CTgList *CTgList::NewL(const TRect &aRect, CTgAppUi &aUi, TBool aIcons)
{
    CTgList *self = new (ELeave) CTgList(aUi, aIcons);
    CleanupStack::PushL(self);
    self->ConstructL(aRect);
    CleanupStack::Pop(self);
    return self;
}

TKeyResponse CTgList::OfferKeyEventL(const TKeyEvent &aKey, TEventCode aType)
{
    if (Searching() && aType == EEventKey && aKey.iCode != EKeyOK && aKey.iCode != EKeyEnter &&
        aKey.iCode != EKeyUpArrow && aKey.iCode != EKeyDownArrow) {
        TBool redraw = EFalse;
        if (AknFind::HandleFindOfferKeyEventL(aKey, aType, this, iListBox, iFind, ETrue, redraw) ==
            EKeyWasConsumed)
            return EKeyWasConsumed;
    }
    if (aType == EEventKey) {
        if (aKey.iCode == EKeyOK || aKey.iCode == EKeyEnter) {
            iUi.SelectL();
            return EKeyWasConsumed;
        }
        if (iUi.ShortcutL(aKey.iCode))
            return EKeyWasConsumed;
    }
    if (iFind && !Searching()) {
        TBool redraw = EFalse;
        if (AknFind::HandleFindOfferKeyEventL(aKey, aType, this, iListBox, iFind, ETrue, redraw) ==
            EKeyWasConsumed)
            return EKeyWasConsumed;
    }
    return iListBox->OfferKeyEventL(aKey, aType);
}

void CTgList::HandleListBoxEventL(CEikListBox *, TListBoxEvent aEvent)
{
    if (aEvent == EEventItemDoubleClicked)
        iUi.SelectL();
}

class CTgDocument : public CAknDocument
{
public:
    CTgDocument(CEikApplication &aApp) : CAknDocument(aApp) {}
    CEikAppUi *CreateAppUiL() { return new (ELeave) CTgAppUi; }
};

class CTgApplication : public CAknApplication
{
    CApaDocument *CreateDocumentL() { return new (ELeave) CTgDocument(*this); }
    TUid AppDllUid() const { return TUid::Uid(0xE7E1E001); }
};

LOCAL_C CApaApplication *NewApplication() { return new CTgApplication; }
GLDEF_C TInt E32Main() { return EikStart::RunApplication(NewApplication); }
