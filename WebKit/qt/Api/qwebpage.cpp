/*
    Copyright (C) 2008, 2009 Nokia Corporation and/or its subsidiary(-ies)
    Copyright (C) 2007 Staikos Computing Services Inc.
    Copyright (C) 2007 Apple Inc.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "config.h"
#include "qwebpage.h"
#include "qwebview.h"
#include "qwebframe.h"
#include "qwebpage_p.h"
#include "qwebframe_p.h"
#include "qwebhistory.h"
#include "qwebhistory_p.h"
#include "qwebinspector.h"
#include "qwebinspector_p.h"
#include "qwebsettings.h"
#include "qwebkitversion.h"

#include "Chrome.h"
#include "ContextMenuController.h"
#include "Frame.h"
#include "FrameTree.h"
#include "FrameLoader.h"
#include "FrameLoaderClientQt.h"
#include "FrameView.h"
#include "FormState.h"
#include "ApplicationCacheStorage.h"
#include "ChromeClientQt.h"
#include "ContextMenu.h"
#include "ContextMenuClientQt.h"
#include "DocumentLoader.h"
#include "DragClientQt.h"
#include "DragController.h"
#include "DragData.h"
#include "EditorClientQt.h"
#include "SecurityOrigin.h"
#include "Settings.h"
#include "Page.h"
#include "Pasteboard.h"
#include "FrameLoader.h"
#include "FrameLoadRequest.h"
#include "KURL.h"
#include "Logging.h"
#include "Image.h"
#include "InspectorClientQt.h"
#include "InspectorController.h"
#include "FocusController.h"
#include "Editor.h"
#include "Scrollbar.h"
#include "PlatformKeyboardEvent.h"
#include "PlatformWheelEvent.h"
#include "PluginDatabase.h"
#include "ProgressTracker.h"
#include "RefPtr.h"
#include "RenderTextControl.h"
#include "TextIterator.h"
#include "HashMap.h"
#include "HTMLFormElement.h"
#include "HTMLInputElement.h"
#include "HTMLNames.h"
#include "HitTestResult.h"
#include "WindowFeatures.h"
#include "LocalizedStrings.h"
#include "Cache.h"
#include "runtime/InitializeThreading.h"
#include "PageGroup.h"
#include "NotificationPresenterClientQt.h"
#include "PageClientQt.h"
#include "WorkerThread.h"
#include "wtf/Threading.h"

#include <QApplication>
#include <QBasicTimer>
#include <QBitArray>
#include <QDebug>
#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QHttpRequestHeader>
#include <QInputDialog>
#include <QLocale>
#include <QMessageBox>
#include <QNetworkProxy>
#include <QUndoStack>
#include <QUrl>
#include <QPainter>
#include <QClipboard>
#include <QSslSocket>
#include <QStyle>
#include <QSysInfo>
#include <QTextCharFormat>
#include <QTextDocument>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#if defined(Q_WS_X11)
#include <QX11Info>
#endif

#if QT_VERSION >= QT_VERSION_CHECK(4, 6, 0)
#include <QTouchEvent>
#include "PlatformTouchEvent.h"
#endif

using namespace WebCore;

bool QWebPagePrivate::drtRun = false;

// Lookup table mapping QWebPage::WebActions to the associated Editor commands
static const char* editorCommandWebActions[] =
{
    0, // OpenLink,

    0, // OpenLinkInNewWindow,
    0, // OpenFrameInNewWindow,

    0, // DownloadLinkToDisk,
    0, // CopyLinkToClipboard,

    0, // OpenImageInNewWindow,
    0, // DownloadImageToDisk,
    0, // CopyImageToClipboard,

    0, // Back,
    0, // Forward,
    0, // Stop,
    0, // Reload,

    "Cut", // Cut,
    "Copy", // Copy,
    "Paste", // Paste,

    "Undo", // Undo,
    "Redo", // Redo,
    "MoveForward", // MoveToNextChar,
    "MoveBackward", // MoveToPreviousChar,
    "MoveWordForward", // MoveToNextWord,
    "MoveWordBackward", // MoveToPreviousWord,
    "MoveDown", // MoveToNextLine,
    "MoveUp", // MoveToPreviousLine,
    "MoveToBeginningOfLine", // MoveToStartOfLine,
    "MoveToEndOfLine", // MoveToEndOfLine,
    "MoveToBeginningOfParagraph", // MoveToStartOfBlock,
    "MoveToEndOfParagraph", // MoveToEndOfBlock,
    "MoveToBeginningOfDocument", // MoveToStartOfDocument,
    "MoveToEndOfDocument", // MoveToEndOfDocument,
    "MoveForwardAndModifySelection", // SelectNextChar,
    "MoveBackwardAndModifySelection", // SelectPreviousChar,
    "MoveWordForwardAndModifySelection", // SelectNextWord,
    "MoveWordBackwardAndModifySelection", // SelectPreviousWord,
    "MoveDownAndModifySelection", // SelectNextLine,
    "MoveUpAndModifySelection", // SelectPreviousLine,
    "MoveToBeginningOfLineAndModifySelection", // SelectStartOfLine,
    "MoveToEndOfLineAndModifySelection", // SelectEndOfLine,
    "MoveToBeginningOfParagraphAndModifySelection", // SelectStartOfBlock,
    "MoveToEndOfParagraphAndModifySelection", // SelectEndOfBlock,
    "MoveToBeginningOfDocumentAndModifySelection", //SelectStartOfDocument,
    "MoveToEndOfDocumentAndModifySelection", // SelectEndOfDocument,
    "DeleteWordBackward", // DeleteStartOfWord,
    "DeleteWordForward", // DeleteEndOfWord,

    0, // SetTextDirectionDefault,
    0, // SetTextDirectionLeftToRight,
    0, // SetTextDirectionRightToLeft,

    "ToggleBold", // ToggleBold,
    "ToggleItalic", // ToggleItalic,
    "ToggleUnderline", // ToggleUnderline,

    0, // InspectElement,

    "InsertNewline", // InsertParagraphSeparator
    "InsertLineBreak", // InsertLineSeparator

    "SelectAll", // SelectAll
    0, // ReloadAndBypassCache,

    "PasteAndMatchStyle", // PasteAndMatchStyle
    "RemoveFormat", // RemoveFormat
    "Strikethrough", // ToggleStrikethrough,
    "Subscript", // ToggleSubscript
    "Superscript", // ToggleSuperscript
    "InsertUnorderedList", // InsertUnorderedList
    "InsertOrderedList", // InsertOrderedList
    "Indent", // Indent
    "Outdent", // Outdent,

    "AlignCenter", // AlignCenter,
    "AlignJustified", // AlignJustified,
    "AlignLeft", // AlignLeft,
    "AlignRight", // AlignRight,

    0 // WebActionCount
};

// Lookup the appropriate editor command to use for WebAction \a action
const char* QWebPagePrivate::editorCommandForWebActions(QWebPage::WebAction action)
{
    if ((action > QWebPage::NoWebAction) && (action < int(sizeof(editorCommandWebActions) / sizeof(const char*))))
        return editorCommandWebActions[action];
    return 0;
}

// If you change this make sure to also adjust the docs for QWebPage::userAgentForUrl
#define WEBKIT_VERSION "527+"

static inline DragOperation dropActionToDragOp(Qt::DropActions actions)
{
    unsigned result = 0;
    if (actions & Qt::CopyAction)
        result |= DragOperationCopy;
    // DragOperationgeneric represents InternetExplorer's equivalent of Move operation,
    // hence it should be considered as "move"
    if (actions & Qt::MoveAction)
        result |= (DragOperationMove | DragOperationGeneric);
    if (actions & Qt::LinkAction)
        result |= DragOperationLink;
    return (DragOperation)result;
}

static inline Qt::DropAction dragOpToDropAction(unsigned actions)
{
    Qt::DropAction result = Qt::IgnoreAction;
    if (actions & DragOperationCopy)
        result = Qt::CopyAction;
    else if (actions & DragOperationMove)
        result = Qt::MoveAction;
    // DragOperationgeneric represents InternetExplorer's equivalent of Move operation,
    // hence it should be considered as "move"
    else if (actions & DragOperationGeneric)
        result = Qt::MoveAction;
    else if (actions & DragOperationLink)
        result = Qt::LinkAction;
    return result;
}

QWebPagePrivate::QWebPagePrivate(QWebPage *qq)
    : q(qq)
    , client(0)
#if QT_VERSION < 0x040600
    , view(0)
#endif
    , clickCausedFocus(false)
    , viewportSize(QSize(0, 0))
    , inspectorFrontend(0)
    , inspector(0)
    , inspectorIsInternalOnly(false)
{
    WebCore::InitializeLoggingChannelsIfNecessary();
    JSC::initializeThreading();
    WTF::initializeMainThread();
    WebCore::SecurityOrigin::setLocalLoadPolicy(WebCore::SecurityOrigin::AllowLocalLoadsForLocalAndSubstituteData);
#if QT_VERSION < QT_VERSION_CHECK(4, 7, 0)
    WebCore::Font::setCodePath(WebCore::Font::Complex);
#endif

    chromeClient = new ChromeClientQt(q);
    contextMenuClient = new ContextMenuClientQt();
    editorClient = new EditorClientQt(q);
    page = new Page(chromeClient, contextMenuClient, editorClient,
                    new DragClientQt(q), new InspectorClientQt(q), 0, 0, 0);

    settings = new QWebSettings(page->settings());

#ifndef QT_NO_UNDOSTACK
    undoStack = 0;
#endif
    mainFrame = 0;
    networkManager = 0;
    pluginFactory = 0;
    insideOpenCall = false;
    forwardUnsupportedContent = false;
    editable = false;
    useFixedLayout = false;
    linkPolicy = QWebPage::DontDelegateLinks;
#ifndef QT_NO_CONTEXTMENU
    currentContextMenu = 0;
#endif
    smartInsertDeleteEnabled = true;
    selectTrailingWhitespaceEnabled = false;

    history.d = new QWebHistoryPrivate(page->backForwardList());
    memset(actions, 0, sizeof(actions));

    PageGroup::setShouldTrackVisitedLinks(true);
    
#if ENABLE(NOTIFICATIONS)    
    NotificationPresenterClientQt::notificationPresenter()->addClient();
#endif
}

QWebPagePrivate::~QWebPagePrivate()
{
#ifndef QT_NO_CONTEXTMENU
    delete currentContextMenu;
#endif
#ifndef QT_NO_UNDOSTACK
    delete undoStack;
#endif
    delete settings;
    delete page;
    
#if ENABLE(NOTIFICATIONS)
    NotificationPresenterClientQt::notificationPresenter()->removeClient();
#endif
}

WebCore::Page* QWebPagePrivate::core(QWebPage* page)
{
    return page->d->page;
}

QWebPagePrivate* QWebPagePrivate::priv(QWebPage* page)
{
    return page->d;
}

bool QWebPagePrivate::acceptNavigationRequest(QWebFrame *frame, const QNetworkRequest &request, QWebPage::NavigationType type)
{
    if (insideOpenCall
        && frame == mainFrame)
        return true;
    return q->acceptNavigationRequest(frame, request, type);
}

void QWebPagePrivate::createMainFrame()
{
    if (!mainFrame) {
        QWebFrameData frameData(page);
        mainFrame = new QWebFrame(q, &frameData);

        emit q->frameCreated(mainFrame);
    }
}

static QWebPage::WebAction webActionForContextMenuAction(WebCore::ContextMenuAction action)
{
    switch (action) {
        case WebCore::ContextMenuItemTagOpenLink: return QWebPage::OpenLink;
        case WebCore::ContextMenuItemTagOpenLinkInNewWindow: return QWebPage::OpenLinkInNewWindow;
        case WebCore::ContextMenuItemTagDownloadLinkToDisk: return QWebPage::DownloadLinkToDisk;
        case WebCore::ContextMenuItemTagCopyLinkToClipboard: return QWebPage::CopyLinkToClipboard;
        case WebCore::ContextMenuItemTagOpenImageInNewWindow: return QWebPage::OpenImageInNewWindow;
        case WebCore::ContextMenuItemTagDownloadImageToDisk: return QWebPage::DownloadImageToDisk;
        case WebCore::ContextMenuItemTagCopyImageToClipboard: return QWebPage::CopyImageToClipboard;
        case WebCore::ContextMenuItemTagOpenFrameInNewWindow: return QWebPage::OpenFrameInNewWindow;
        case WebCore::ContextMenuItemTagCopy: return QWebPage::Copy;
        case WebCore::ContextMenuItemTagGoBack: return QWebPage::Back;
        case WebCore::ContextMenuItemTagGoForward: return QWebPage::Forward;
        case WebCore::ContextMenuItemTagStop: return QWebPage::Stop;
        case WebCore::ContextMenuItemTagReload: return QWebPage::Reload;
        case WebCore::ContextMenuItemTagCut: return QWebPage::Cut;
        case WebCore::ContextMenuItemTagPaste: return QWebPage::Paste;
        case WebCore::ContextMenuItemTagDefaultDirection: return QWebPage::SetTextDirectionDefault;
        case WebCore::ContextMenuItemTagLeftToRight: return QWebPage::SetTextDirectionLeftToRight;
        case WebCore::ContextMenuItemTagRightToLeft: return QWebPage::SetTextDirectionRightToLeft;
        case WebCore::ContextMenuItemTagBold: return QWebPage::ToggleBold;
        case WebCore::ContextMenuItemTagItalic: return QWebPage::ToggleItalic;
        case WebCore::ContextMenuItemTagUnderline: return QWebPage::ToggleUnderline;
#if ENABLE(INSPECTOR)
        case WebCore::ContextMenuItemTagInspectElement: return QWebPage::InspectElement;
#endif
        default: break;
    }
    return QWebPage::NoWebAction;
}

#ifndef QT_NO_CONTEXTMENU
QMenu *QWebPagePrivate::createContextMenu(const WebCore::ContextMenu *webcoreMenu,
        const QList<WebCore::ContextMenuItem> *items, QBitArray *visitedWebActions)
{
    if (!client)
        return 0;

    QMenu* menu = new QMenu(client->ownerWidget());
    for (int i = 0; i < items->count(); ++i) {
        const ContextMenuItem &item = items->at(i);
        switch (item.type()) {
            case WebCore::CheckableActionType: /* fall through */
            case WebCore::ActionType: {
                QWebPage::WebAction action = webActionForContextMenuAction(item.action());
                QAction *a = q->action(action);
                if (a) {
                    ContextMenuItem it(item);
                    webcoreMenu->checkOrEnableIfNeeded(it);
                    PlatformMenuItemDescription desc = it.releasePlatformDescription();
                    a->setEnabled(desc.enabled);
                    a->setChecked(desc.checked);
                    a->setCheckable(item.type() == WebCore::CheckableActionType);

                    menu->addAction(a);
                    visitedWebActions->setBit(action);
                }
                break;
            }
            case WebCore::SeparatorType:
                menu->addSeparator();
                break;
            case WebCore::SubmenuType: {
                QMenu *subMenu = createContextMenu(webcoreMenu, item.platformSubMenu(), visitedWebActions);

                bool anyEnabledAction = false;

                QList<QAction *> actions = subMenu->actions();
                for (int i = 0; i < actions.count(); ++i) {
                    if (actions.at(i)->isVisible())
                        anyEnabledAction |= actions.at(i)->isEnabled();
                }

                // don't show sub-menus with just disabled actions
                if (anyEnabledAction) {
                    subMenu->setTitle(item.title());
                    menu->addAction(subMenu->menuAction());
                } else
                    delete subMenu;
                break;
            }
        }
    }
    return menu;
}
#endif // QT_NO_CONTEXTMENU

#ifndef QT_NO_ACTION
void QWebPagePrivate::_q_webActionTriggered(bool checked)
{
    QAction *a = qobject_cast<QAction *>(q->sender());
    if (!a)
        return;
    QWebPage::WebAction action = static_cast<QWebPage::WebAction>(a->data().toInt());
    q->triggerAction(action, checked);
}
#endif // QT_NO_ACTION

void QWebPagePrivate::_q_cleanupLeakMessages()
{
#ifndef NDEBUG
    // Need this to make leak messages accurate.
    cache()->setCapacities(0, 0, 0);
#endif
}

void QWebPagePrivate::updateAction(QWebPage::WebAction action)
{
#ifdef QT_NO_ACTION
    Q_UNUSED(action)
#else
    QAction *a = actions[action];
    if (!a || !mainFrame)
        return;

    WebCore::FrameLoader *loader = mainFrame->d->frame->loader();
    WebCore::Editor *editor = page->focusController()->focusedOrMainFrame()->editor();

    bool enabled = a->isEnabled();
    bool checked = a->isChecked();

    switch (action) {
        case QWebPage::Back:
            enabled = page->canGoBackOrForward(-1);
            break;
        case QWebPage::Forward:
            enabled = page->canGoBackOrForward(1);
            break;
        case QWebPage::Stop:
            enabled = loader->isLoading();
            break;
        case QWebPage::Reload:
        case QWebPage::ReloadAndBypassCache:
            enabled = !loader->isLoading();
            break;
#ifndef QT_NO_UNDOSTACK
        case QWebPage::Undo:
        case QWebPage::Redo:
            // those two are handled by QUndoStack
            break;
#endif // QT_NO_UNDOSTACK
        case QWebPage::SelectAll: // editor command is always enabled
            break;
        case QWebPage::SetTextDirectionDefault:
        case QWebPage::SetTextDirectionLeftToRight:
        case QWebPage::SetTextDirectionRightToLeft:
            enabled = editor->canEdit();
            checked = false;
            break;
        default: {
            // see if it's an editor command
            const char* commandName = editorCommandForWebActions(action);

            // if it's an editor command, let it's logic determine state
            if (commandName) {
                Editor::Command command = editor->command(commandName);
                enabled = command.isEnabled();
                if (enabled)
                    checked = command.state() != FalseTriState;
                else
                    checked = false;
            }
            break;
        }
    }

    a->setEnabled(enabled);

    if (a->isCheckable())
        a->setChecked(checked);
#endif // QT_NO_ACTION
}

void QWebPagePrivate::updateNavigationActions()
{
    updateAction(QWebPage::Back);
    updateAction(QWebPage::Forward);
    updateAction(QWebPage::Stop);
    updateAction(QWebPage::Reload);
    updateAction(QWebPage::ReloadAndBypassCache);
}

void QWebPagePrivate::updateEditorActions()
{
    updateAction(QWebPage::Cut);
    updateAction(QWebPage::Copy);
    updateAction(QWebPage::Paste);
    updateAction(QWebPage::MoveToNextChar);
    updateAction(QWebPage::MoveToPreviousChar);
    updateAction(QWebPage::MoveToNextWord);
    updateAction(QWebPage::MoveToPreviousWord);
    updateAction(QWebPage::MoveToNextLine);
    updateAction(QWebPage::MoveToPreviousLine);
    updateAction(QWebPage::MoveToStartOfLine);
    updateAction(QWebPage::MoveToEndOfLine);
    updateAction(QWebPage::MoveToStartOfBlock);
    updateAction(QWebPage::MoveToEndOfBlock);
    updateAction(QWebPage::MoveToStartOfDocument);
    updateAction(QWebPage::MoveToEndOfDocument);
    updateAction(QWebPage::SelectNextChar);
    updateAction(QWebPage::SelectPreviousChar);
    updateAction(QWebPage::SelectNextWord);
    updateAction(QWebPage::SelectPreviousWord);
    updateAction(QWebPage::SelectNextLine);
    updateAction(QWebPage::SelectPreviousLine);
    updateAction(QWebPage::SelectStartOfLine);
    updateAction(QWebPage::SelectEndOfLine);
    updateAction(QWebPage::SelectStartOfBlock);
    updateAction(QWebPage::SelectEndOfBlock);
    updateAction(QWebPage::SelectStartOfDocument);
    updateAction(QWebPage::SelectEndOfDocument);
    updateAction(QWebPage::DeleteStartOfWord);
    updateAction(QWebPage::DeleteEndOfWord);
    updateAction(QWebPage::SetTextDirectionDefault);
    updateAction(QWebPage::SetTextDirectionLeftToRight);
    updateAction(QWebPage::SetTextDirectionRightToLeft);
    updateAction(QWebPage::ToggleBold);
    updateAction(QWebPage::ToggleItalic);
    updateAction(QWebPage::ToggleUnderline);
    updateAction(QWebPage::InsertParagraphSeparator);
    updateAction(QWebPage::InsertLineSeparator);
    updateAction(QWebPage::PasteAndMatchStyle);
    updateAction(QWebPage::RemoveFormat);
    updateAction(QWebPage::ToggleStrikethrough);
    updateAction(QWebPage::ToggleSubscript);
    updateAction(QWebPage::ToggleSuperscript);
    updateAction(QWebPage::InsertUnorderedList);
    updateAction(QWebPage::InsertOrderedList);
    updateAction(QWebPage::Indent);
    updateAction(QWebPage::Outdent);
    updateAction(QWebPage::AlignCenter);
    updateAction(QWebPage::AlignJustified);
    updateAction(QWebPage::AlignLeft);
    updateAction(QWebPage::AlignRight);
}

void QWebPagePrivate::timerEvent(QTimerEvent *ev)
{
    int timerId = ev->timerId();
    if (timerId == tripleClickTimer.timerId())
        tripleClickTimer.stop();
    else
        q->timerEvent(ev);
}

void QWebPagePrivate::mouseMoveEvent(QGraphicsSceneMouseEvent* ev)
{
    WebCore::Frame* frame = QWebFramePrivate::core(mainFrame);
    if (!frame->view())
        return;

    bool accepted = frame->eventHandler()->mouseMoved(PlatformMouseEvent(ev, 0));
    ev->setAccepted(accepted);
}

void QWebPagePrivate::mouseMoveEvent(QMouseEvent *ev)
{
    WebCore::Frame* frame = QWebFramePrivate::core(mainFrame);
    if (!frame->view())
        return;

    bool accepted = frame->eventHandler()->mouseMoved(PlatformMouseEvent(ev, 0));
    ev->setAccepted(accepted);
}

void QWebPagePrivate::mousePressEvent(QGraphicsSceneMouseEvent* ev)
{
    WebCore::Frame* frame = QWebFramePrivate::core(mainFrame);
    if (!frame->view())
        return;

    RefPtr<WebCore::Node> oldNode;
    Frame* focusedFrame = page->focusController()->focusedFrame();
    if (Document* focusedDocument = focusedFrame ? focusedFrame->document() : 0)
        oldNode = focusedDocument->focusedNode();

    if (tripleClickTimer.isActive()
            && (ev->pos().toPoint() - tripleClick).manhattanLength()
                < QApplication::startDragDistance()) {
        mouseTripleClickEvent(ev);
        return;
    }

    bool accepted = false;
    PlatformMouseEvent mev(ev, 1);
    // ignore the event if we can't map Qt's mouse buttons to WebCore::MouseButton
    if (mev.button() != NoButton)
        accepted = frame->eventHandler()->handleMousePressEvent(mev);
    ev->setAccepted(accepted);

    RefPtr<WebCore::Node> newNode;
    focusedFrame = page->focusController()->focusedFrame();
    if (Document* focusedDocument = focusedFrame ? focusedFrame->document() : 0)
        newNode = focusedDocument->focusedNode();

    if (newNode && oldNode != newNode)
        clickCausedFocus = true;
}

void QWebPagePrivate::mousePressEvent(QMouseEvent *ev)
{
    WebCore::Frame* frame = QWebFramePrivate::core(mainFrame);
    if (!frame->view())
        return;

    RefPtr<WebCore::Node> oldNode;
    Frame* focusedFrame = page->focusController()->focusedFrame();
    if (Document* focusedDocument = focusedFrame ? focusedFrame->document() : 0)
        oldNode = focusedDocument->focusedNode();

    if (tripleClickTimer.isActive()
            && (ev->pos() - tripleClick).manhattanLength()
                < QApplication::startDragDistance()) {
        mouseTripleClickEvent(ev);
        return;
    }

    bool accepted = false;
    PlatformMouseEvent mev(ev, 1);
    // ignore the event if we can't map Qt's mouse buttons to WebCore::MouseButton
    if (mev.button() != NoButton)
        accepted = frame->eventHandler()->handleMousePressEvent(mev);
    ev->setAccepted(accepted);

    RefPtr<WebCore::Node> newNode;
    focusedFrame = page->focusController()->focusedFrame();
    if (Document* focusedDocument = focusedFrame ? focusedFrame->document() : 0)
        newNode = focusedDocument->focusedNode();

    if (newNode && oldNode != newNode)
        clickCausedFocus = true;
}

void QWebPagePrivate::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *ev)
{
    WebCore::Frame* frame = QWebFramePrivate::core(mainFrame);
    if (!frame->view())
        return;

    bool accepted = false;
    PlatformMouseEvent mev(ev, 2);
    // ignore the event if we can't map Qt's mouse buttons to WebCore::MouseButton
    if (mev.button() != NoButton)
        accepted = frame->eventHandler()->handleMousePressEvent(mev);
    ev->setAccepted(accepted);

    tripleClickTimer.start(QApplication::doubleClickInterval(), q);
    tripleClick = ev->pos().toPoint();
}

void QWebPagePrivate::mouseDoubleClickEvent(QMouseEvent *ev)
{
    WebCore::Frame* frame = QWebFramePrivate::core(mainFrame);
    if (!frame->view())
        return;

    bool accepted = false;
    PlatformMouseEvent mev(ev, 2);
    // ignore the event if we can't map Qt's mouse buttons to WebCore::MouseButton
    if (mev.button() != NoButton)
        accepted = frame->eventHandler()->handleMousePressEvent(mev);
    ev->setAccepted(accepted);

    tripleClickTimer.start(QApplication::doubleClickInterval(), q);
    tripleClick = ev->pos();
}

void QWebPagePrivate::mouseTripleClickEvent(QGraphicsSceneMouseEvent *ev)
{
    WebCore::Frame* frame = QWebFramePrivate::core(mainFrame);
    if (!frame->view())
        return;

    bool accepted = false;
    PlatformMouseEvent mev(ev, 3);
    // ignore the event if we can't map Qt's mouse buttons to WebCore::MouseButton
    if (mev.button() != NoButton)
        accepted = frame->eventHandler()->handleMousePressEvent(mev);
    ev->setAccepted(accepted);
}

void QWebPagePrivate::mouseTripleClickEvent(QMouseEvent *ev)
{
    WebCore::Frame* frame = QWebFramePrivate::core(mainFrame);
    if (!frame->view())
        return;

    bool accepted = false;
    PlatformMouseEvent mev(ev, 3);
    // ignore the event if we can't map Qt's mouse buttons to WebCore::MouseButton
    if (mev.button() != NoButton)
        accepted = frame->eventHandler()->handleMousePressEvent(mev);
    ev->setAccepted(accepted);
}

void QWebPagePrivate::handleClipboard(QEvent* ev, Qt::MouseButton button)
{
#ifndef QT_NO_CLIPBOARD
    if (QApplication::clipboard()->supportsSelection()) {
        bool oldSelectionMode = Pasteboard::generalPasteboard()->isSelectionMode();
        Pasteboard::generalPasteboard()->setSelectionMode(true);
        WebCore::Frame* focusFrame = page->focusController()->focusedOrMainFrame();
        if (button == Qt::LeftButton) {
            if (focusFrame && (focusFrame->editor()->canCopy() || focusFrame->editor()->canDHTMLCopy())) {
                focusFrame->editor()->copy();
                ev->setAccepted(true);
            }
        } else if (button == Qt::MidButton) {
            if (focusFrame && (focusFrame->editor()->canPaste() || focusFrame->editor()->canDHTMLPaste())) {
                focusFrame->editor()->paste();
                ev->setAccepted(true);
            }
        }
        Pasteboard::generalPasteboard()->setSelectionMode(oldSelectionMode);
    }
#endif
}

void QWebPagePrivate::mouseReleaseEvent(QGraphicsSceneMouseEvent* ev)
{
    WebCore::Frame* frame = QWebFramePrivate::core(mainFrame);
    if (!frame->view())
        return;

    bool accepted = false;
    PlatformMouseEvent mev(ev, 0);
    // ignore the event if we can't map Qt's mouse buttons to WebCore::MouseButton
    if (mev.button() != NoButton)
        accepted = frame->eventHandler()->handleMouseReleaseEvent(mev);
    ev->setAccepted(accepted);

    handleClipboard(ev, ev->button());
    handleSoftwareInputPanel(ev->button());
}

void QWebPagePrivate::handleSoftwareInputPanel(Qt::MouseButton button)
{
#if QT_VERSION >= QT_VERSION_CHECK(4, 6, 0)
    Frame* frame = page->focusController()->focusedFrame();
    if (!frame)
        return;

    if (client && client->inputMethodEnabled()
        && frame->document()->focusedNode()
        && button == Qt::LeftButton && qApp->autoSipEnabled()) {
        QStyle::RequestSoftwareInputPanel behavior = QStyle::RequestSoftwareInputPanel(
            client->ownerWidget()->style()->styleHint(QStyle::SH_RequestSoftwareInputPanel));
        if (!clickCausedFocus || behavior == QStyle::RSIP_OnMouseClick) {
            QEvent event(QEvent::RequestSoftwareInputPanel);
            QApplication::sendEvent(client->ownerWidget(), &event);
        }
    }

    clickCausedFocus = false;
#endif
}

void QWebPagePrivate::mouseReleaseEvent(QMouseEvent *ev)
{
    WebCore::Frame* frame = QWebFramePrivate::core(mainFrame);
    if (!frame->view())
        return;

    bool accepted = false;
    PlatformMouseEvent mev(ev, 0);
    // ignore the event if we can't map Qt's mouse buttons to WebCore::MouseButton
    if (mev.button() != NoButton)
        accepted = frame->eventHandler()->handleMouseReleaseEvent(mev);
    ev->setAccepted(accepted);

    handleClipboard(ev, ev->button());
    handleSoftwareInputPanel(ev->button());
}

#ifndef QT_NO_CONTEXTMENU
void QWebPagePrivate::contextMenuEvent(const QPoint& globalPos)
{
    QMenu *menu = q->createStandardContextMenu();
    if (menu) {
        menu->exec(globalPos);
        delete menu;
    }
}
#endif // QT_NO_CONTEXTMENU

/*!
    \since 4.5
    This function creates the standard context menu which is shown when
    the user clicks on the web page with the right mouse button. It is
    called from the default contextMenuEvent() handler. The popup menu's
    ownership is transferred to the caller.
 */
QMenu *QWebPage::createStandardContextMenu()
{
#ifndef QT_NO_CONTEXTMENU
    QMenu *menu = d->currentContextMenu;
    d->currentContextMenu = 0;
    return menu;
#else
    return 0;
#endif
}

#ifndef QT_NO_WHEELEVENT
void QWebPagePrivate::wheelEvent(QGraphicsSceneWheelEvent* ev)
{
    WebCore::Frame* frame = QWebFramePrivate::core(mainFrame);
    if (!frame->view())
        return;

    WebCore::PlatformWheelEvent pev(ev);
    bool accepted = frame->eventHandler()->handleWheelEvent(pev);
    ev->setAccepted(accepted);
}

void QWebPagePrivate::wheelEvent(QWheelEvent *ev)
{
    WebCore::Frame* frame = QWebFramePrivate::core(mainFrame);
    if (!frame->view())
        return;

    WebCore::PlatformWheelEvent pev(ev);
    bool accepted = frame->eventHandler()->handleWheelEvent(pev);
    ev->setAccepted(accepted);
}
#endif // QT_NO_WHEELEVENT

#ifndef QT_NO_SHORTCUT
QWebPage::WebAction QWebPagePrivate::editorActionForKeyEvent(QKeyEvent* event)
{
    static struct {
        QKeySequence::StandardKey standardKey;
        QWebPage::WebAction action;
    } editorActions[] = {
        { QKeySequence::Cut, QWebPage::Cut },
        { QKeySequence::Copy, QWebPage::Copy },
        { QKeySequence::Paste, QWebPage::Paste },
        { QKeySequence::Undo, QWebPage::Undo },
        { QKeySequence::Redo, QWebPage::Redo },
        { QKeySequence::MoveToNextChar, QWebPage::MoveToNextChar },
        { QKeySequence::MoveToPreviousChar, QWebPage::MoveToPreviousChar },
        { QKeySequence::MoveToNextWord, QWebPage::MoveToNextWord },
        { QKeySequence::MoveToPreviousWord, QWebPage::MoveToPreviousWord },
        { QKeySequence::MoveToNextLine, QWebPage::MoveToNextLine },
        { QKeySequence::MoveToPreviousLine, QWebPage::MoveToPreviousLine },
        { QKeySequence::MoveToStartOfLine, QWebPage::MoveToStartOfLine },
        { QKeySequence::MoveToEndOfLine, QWebPage::MoveToEndOfLine },
        { QKeySequence::MoveToStartOfBlock, QWebPage::MoveToStartOfBlock },
        { QKeySequence::MoveToEndOfBlock, QWebPage::MoveToEndOfBlock },
        { QKeySequence::MoveToStartOfDocument, QWebPage::MoveToStartOfDocument },
        { QKeySequence::MoveToEndOfDocument, QWebPage::MoveToEndOfDocument },
        { QKeySequence::SelectNextChar, QWebPage::SelectNextChar },
        { QKeySequence::SelectPreviousChar, QWebPage::SelectPreviousChar },
        { QKeySequence::SelectNextWord, QWebPage::SelectNextWord },
        { QKeySequence::SelectPreviousWord, QWebPage::SelectPreviousWord },
        { QKeySequence::SelectNextLine, QWebPage::SelectNextLine },
        { QKeySequence::SelectPreviousLine, QWebPage::SelectPreviousLine },
        { QKeySequence::SelectStartOfLine, QWebPage::SelectStartOfLine },
        { QKeySequence::SelectEndOfLine, QWebPage::SelectEndOfLine },
        { QKeySequence::SelectStartOfBlock, QWebPage::SelectStartOfBlock },
        { QKeySequence::SelectEndOfBlock,  QWebPage::SelectEndOfBlock },
        { QKeySequence::SelectStartOfDocument, QWebPage::SelectStartOfDocument },
        { QKeySequence::SelectEndOfDocument, QWebPage::SelectEndOfDocument },
        { QKeySequence::DeleteStartOfWord, QWebPage::DeleteStartOfWord },
        { QKeySequence::DeleteEndOfWord, QWebPage::DeleteEndOfWord },
        { QKeySequence::InsertParagraphSeparator, QWebPage::InsertParagraphSeparator },
        { QKeySequence::InsertLineSeparator, QWebPage::InsertLineSeparator },
        { QKeySequence::SelectAll, QWebPage::SelectAll },
        { QKeySequence::UnknownKey, QWebPage::NoWebAction }
    };

    for (int i = 0; editorActions[i].standardKey != QKeySequence::UnknownKey; ++i)
        if (event == editorActions[i].standardKey)
            return editorActions[i].action;

    return QWebPage::NoWebAction;
}
#endif // QT_NO_SHORTCUT

void QWebPagePrivate::keyPressEvent(QKeyEvent *ev)
{
    bool handled = false;
    WebCore::Frame* frame = page->focusController()->focusedOrMainFrame();
    // we forward the key event to WebCore first to handle potential DOM
    // defined event handlers and later on end up in EditorClientQt::handleKeyboardEvent
    // to trigger editor commands via triggerAction().
    if (!handled)
        handled = frame->eventHandler()->keyEvent(ev);
    if (!handled) {
        handled = true;
        if (!handleScrolling(ev, frame)) {
            switch (ev->key()) {
            case Qt::Key_Back:
                q->triggerAction(QWebPage::Back);
                break;
            case Qt::Key_Forward:
                q->triggerAction(QWebPage::Forward);
                break;
            case Qt::Key_Stop:
                q->triggerAction(QWebPage::Stop);
                break;
            case Qt::Key_Refresh:
                q->triggerAction(QWebPage::Reload);
                break;
            case Qt::Key_Backspace:
                if (ev->modifiers() == Qt::ShiftModifier)
                    q->triggerAction(QWebPage::Forward);
                else
                    q->triggerAction(QWebPage::Back);
                break;
            default:
                handled = false;
                break;
            }
        }
    }

    ev->setAccepted(handled);
}

void QWebPagePrivate::keyReleaseEvent(QKeyEvent *ev)
{
    if (ev->isAutoRepeat()) {
        ev->setAccepted(true);
        return;
    }

    WebCore::Frame* frame = page->focusController()->focusedOrMainFrame();
    bool handled = frame->eventHandler()->keyEvent(ev);
    ev->setAccepted(handled);
}

void QWebPagePrivate::focusInEvent(QFocusEvent*)
{
    FocusController *focusController = page->focusController();
    focusController->setActive(true);
    focusController->setFocused(true);
    if (!focusController->focusedFrame())
        focusController->setFocusedFrame(QWebFramePrivate::core(mainFrame));
}

void QWebPagePrivate::focusOutEvent(QFocusEvent*)
{
    // only set the focused frame inactive so that we stop painting the caret
    // and the focus frame. But don't tell the focus controller so that upon
    // focusInEvent() we can re-activate the frame.
    FocusController *focusController = page->focusController();
    // Call setFocused first so that window.onblur doesn't get called twice
    focusController->setFocused(false);
    focusController->setActive(false);
}

void QWebPagePrivate::dragEnterEvent(QGraphicsSceneDragDropEvent* ev)
{
#ifndef QT_NO_DRAGANDDROP
    DragData dragData(ev->mimeData(), ev->pos().toPoint(),
            QCursor::pos(), dropActionToDragOp(ev->possibleActions()));
    Qt::DropAction action = dragOpToDropAction(page->dragController()->dragEntered(&dragData));
    ev->setDropAction(action);
    if (action != Qt::IgnoreAction)
        ev->acceptProposedAction();
#endif
}

void QWebPagePrivate::dragEnterEvent(QDragEnterEvent* ev)
{
#ifndef QT_NO_DRAGANDDROP
    DragData dragData(ev->mimeData(), ev->pos(), QCursor::pos(),
                      dropActionToDragOp(ev->possibleActions()));
    Qt::DropAction action = dragOpToDropAction(page->dragController()->dragEntered(&dragData));
    ev->setDropAction(action);
    // We must accept this event in order to receive the drag move events that are sent
    // while the drag and drop action is in progress.
    ev->acceptProposedAction();
#endif
}

void QWebPagePrivate::dragLeaveEvent(QGraphicsSceneDragDropEvent* ev)
{
#ifndef QT_NO_DRAGANDDROP
    DragData dragData(0, IntPoint(), QCursor::pos(), DragOperationNone);
    page->dragController()->dragExited(&dragData);
    ev->accept();
#endif
}

void QWebPagePrivate::dragLeaveEvent(QDragLeaveEvent* ev)
{
#ifndef QT_NO_DRAGANDDROP
    DragData dragData(0, IntPoint(), QCursor::pos(), DragOperationNone);
    page->dragController()->dragExited(&dragData);
    ev->accept();
#endif
}

void QWebPagePrivate::dragMoveEvent(QGraphicsSceneDragDropEvent* ev)
{
#ifndef QT_NO_DRAGANDDROP
    DragData dragData(ev->mimeData(), ev->pos().toPoint(),
            QCursor::pos(), dropActionToDragOp(ev->possibleActions()));
    Qt::DropAction action = dragOpToDropAction(page->dragController()->dragUpdated(&dragData));
    ev->setDropAction(action);
    if (action != Qt::IgnoreAction)
        ev->acceptProposedAction();
#endif
}

void QWebPagePrivate::dragMoveEvent(QDragMoveEvent* ev)
{
#ifndef QT_NO_DRAGANDDROP
    DragData dragData(ev->mimeData(), ev->pos(), QCursor::pos(),
                      dropActionToDragOp(ev->possibleActions()));
    Qt::DropAction action = dragOpToDropAction(page->dragController()->dragUpdated(&dragData));
    m_lastDropAction = action;
    ev->setDropAction(action);
    // We must accept this event in order to receive the drag move events that are sent
    // while the drag and drop action is in progress.
    ev->acceptProposedAction();
#endif
}

void QWebPagePrivate::dropEvent(QGraphicsSceneDragDropEvent* ev)
{
#ifndef QT_NO_DRAGANDDROP
    DragData dragData(ev->mimeData(), ev->pos().toPoint(),
            QCursor::pos(), dropActionToDragOp(ev->possibleActions()));
    if (page->dragController()->performDrag(&dragData))
        ev->acceptProposedAction();
#endif
}

void QWebPagePrivate::dropEvent(QDropEvent* ev)
{
#ifndef QT_NO_DRAGANDDROP
    // Overwrite the defaults set by QDragManager::defaultAction()
    ev->setDropAction(m_lastDropAction);
    DragData dragData(ev->mimeData(), ev->pos(), QCursor::pos(),
                      dropActionToDragOp(Qt::DropAction(ev->dropAction())));
    if (page->dragController()->performDrag(&dragData))
        ev->acceptProposedAction();
#endif
}

void QWebPagePrivate::leaveEvent(QEvent*)
{
    // Fake a mouse move event just outside of the widget, since all
    // the interesting mouse-out behavior like invalidating scrollbars
    // is handled by the WebKit event handler's mouseMoved function.
    QMouseEvent fakeEvent(QEvent::MouseMove, QCursor::pos(), Qt::NoButton, Qt::NoButton, Qt::NoModifier);
    mouseMoveEvent(&fakeEvent);
}

/*!
    \property QWebPage::palette
    \brief the page's palette

    The base brush of the palette is used to draw the background of the main frame.

    By default, this property contains the application's default palette.
*/
void QWebPage::setPalette(const QPalette &pal)
{
    d->palette = pal;
    if (!d->mainFrame || !d->mainFrame->d->frame->view())
        return;

    QBrush brush = pal.brush(QPalette::Base);
    QColor backgroundColor = brush.style() == Qt::SolidPattern ? brush.color() : QColor();
    QWebFramePrivate::core(d->mainFrame)->view()->updateBackgroundRecursively(backgroundColor, !backgroundColor.alpha());
}

QPalette QWebPage::palette() const
{
    return d->palette;
}

void QWebPagePrivate::inputMethodEvent(QInputMethodEvent *ev)
{
    WebCore::Frame *frame = page->focusController()->focusedOrMainFrame();
    WebCore::Editor *editor = frame->editor();
#if QT_VERSION >= 0x040600
    QInputMethodEvent::Attribute selection(QInputMethodEvent::Selection, 0, 0, QVariant());
#endif

    if (!editor->canEdit()) {
        ev->ignore();
        return;
    }

    RenderObject* renderer = 0;
    RenderTextControl* renderTextControl = 0;

    if (frame->selection()->rootEditableElement())
        renderer = frame->selection()->rootEditableElement()->shadowAncestorNode()->renderer();

    if (renderer && renderer->isTextControl())
        renderTextControl = toRenderTextControl(renderer);

    Vector<CompositionUnderline> underlines;
    bool hasSelection = false;

    for (int i = 0; i < ev->attributes().size(); ++i) {
        const QInputMethodEvent::Attribute& a = ev->attributes().at(i);
        switch (a.type) {
        case QInputMethodEvent::TextFormat: {
            QTextCharFormat textCharFormat = a.value.value<QTextFormat>().toCharFormat();
            QColor qcolor = textCharFormat.underlineColor();
            underlines.append(CompositionUnderline(qMin(a.start, (a.start + a.length)), qMax(a.start, (a.start + a.length)), Color(makeRGBA(qcolor.red(), qcolor.green(), qcolor.blue(), qcolor.alpha())), false));
            break;
        }
        case QInputMethodEvent::Cursor: {
            frame->selection()->setCaretVisible(a.length); //if length is 0 cursor is invisible
            if (a.length > 0) {
                RenderObject* caretRenderer = frame->selection()->caretRenderer();
                if (caretRenderer) {
                    QColor qcolor = a.value.value<QColor>();
                    caretRenderer->style()->setColor(Color(makeRGBA(qcolor.red(), qcolor.green(), qcolor.blue(), qcolor.alpha())));
                }
            }
            break;
        }
#if QT_VERSION >= 0x040600
        case QInputMethodEvent::Selection: {
            selection = a;
            hasSelection = true;
            break;
        }
#endif
        }
    }

    if (!ev->commitString().isEmpty())
        editor->confirmComposition(ev->commitString());
    else {
        // 1. empty preedit with a selection attribute, and start/end of 0 cancels composition
        // 2. empty preedit with a selection attribute, and start/end of non-0 updates selection of current preedit text
        // 3. populated preedit with a selection attribute, and start/end of 0 or non-0 updates selection of supplied preedit text
        // 4. otherwise event is updating supplied pre-edit text
        QString preedit = ev->preeditString();
#if QT_VERSION >= 0x040600
        if (hasSelection) {
            QString text = (renderTextControl) ? QString(renderTextControl->text()) : QString();
            if (preedit.isEmpty() && selection.start + selection.length > 0)
                preedit = text;
            editor->setComposition(preedit, underlines,
                                   (selection.length < 0) ? selection.start + selection.length : selection.start,
                                   (selection.length < 0) ? selection.start : selection.start + selection.length);
        } else
#endif
            editor->setComposition(preedit, underlines, preedit.length(), 0);
    }

    ev->accept();
}

#ifndef QT_NO_PROPERTIES
typedef struct {
    const char* name;
    double deferredRepaintDelay;
    double initialDeferredRepaintDelayDuringLoading;
    double maxDeferredRepaintDelayDuringLoading;
    double deferredRepaintDelayIncrementDuringLoading;
} QRepaintThrottlingPreset;

void QWebPagePrivate::dynamicPropertyChangeEvent(QDynamicPropertyChangeEvent* event)
{
    if (event->propertyName() == "_q_viewMode") {
        QString mode = q->property("_q_viewMode").toString();
        if (mode != viewMode) {
            viewMode = mode;
            WebCore::Frame* frame = QWebFramePrivate::core(q->mainFrame());
            WebCore::FrameView* view = frame->view();
            frame->document()->updateStyleSelector();
            view->layout();
        }
    } else if (event->propertyName() == "_q_HTMLTokenizerChunkSize") {
        int chunkSize = q->property("_q_HTMLTokenizerChunkSize").toInt();
        q->handle()->page->setCustomHTMLTokenizerChunkSize(chunkSize);
    } else if (event->propertyName() == "_q_HTMLTokenizerTimeDelay") {
        double timeDelay = q->property("_q_HTMLTokenizerTimeDelay").toDouble();
        q->handle()->page->setCustomHTMLTokenizerTimeDelay(timeDelay);
    } else if (event->propertyName() == "_q_RepaintThrottlingDeferredRepaintDelay") {
        double p = q->property("_q_RepaintThrottlingDeferredRepaintDelay").toDouble();
        FrameView::setRepaintThrottlingDeferredRepaintDelay(p);
    } else if (event->propertyName() == "_q_RepaintThrottlingnInitialDeferredRepaintDelayDuringLoading") {
        double p = q->property("_q_RepaintThrottlingnInitialDeferredRepaintDelayDuringLoading").toDouble();
        FrameView::setRepaintThrottlingnInitialDeferredRepaintDelayDuringLoading(p);
    } else if (event->propertyName() == "_q_RepaintThrottlingMaxDeferredRepaintDelayDuringLoading") {
        double p = q->property("_q_RepaintThrottlingMaxDeferredRepaintDelayDuringLoading").toDouble();
        FrameView::setRepaintThrottlingMaxDeferredRepaintDelayDuringLoading(p);
    } else if (event->propertyName() == "_q_RepaintThrottlingDeferredRepaintDelayIncrementDuringLoading") {
        double p = q->property("_q_RepaintThrottlingDeferredRepaintDelayIncrementDuringLoading").toDouble();
        FrameView::setRepaintThrottlingDeferredRepaintDelayIncrementDuringLoading(p);
    } else if (event->propertyName() == "_q_RepaintThrottlingPreset") {
        static const QRepaintThrottlingPreset presets[] = {
            {   "NoThrottling",     0,      0,      0,      0 },
            {   "Legacy",       0.025,      0,    2.5,    0.5 },
            {   "Minimal",       0.01,      0,      1,    0.2 },
            {   "Medium",       0.025,      1,      5,    0.5 },
            {   "Heavy",          0.1,      2,     10,      1 }
        };

        QString p = q->property("_q_RepaintThrottlingPreset").toString();
        for(int i = 0; i < sizeof(presets) / sizeof(presets[0]); i++) {
            if(p == presets[i].name) {
                FrameView::setRepaintThrottlingDeferredRepaintDelay(
                        presets[i].deferredRepaintDelay);
                FrameView::setRepaintThrottlingnInitialDeferredRepaintDelayDuringLoading(
                        presets[i].initialDeferredRepaintDelayDuringLoading);
                FrameView::setRepaintThrottlingMaxDeferredRepaintDelayDuringLoading(
                        presets[i].maxDeferredRepaintDelayDuringLoading);
                FrameView::setRepaintThrottlingDeferredRepaintDelayIncrementDuringLoading(
                        presets[i].deferredRepaintDelayIncrementDuringLoading);
                break;
            }
        }
    }
#if ENABLE(TILED_BACKING_STORE)
    else if (event->propertyName() == "_q_TiledBackingStoreTileSize") {
        WebCore::Frame* frame = QWebFramePrivate::core(q->mainFrame());
        if (!frame->tiledBackingStore())
            return;
        QSize tileSize = q->property("_q_TiledBackingStoreTileSize").toSize();
        frame->tiledBackingStore()->setTileSize(tileSize);
    } else if (event->propertyName() == "_q_TiledBackingStoreTileCreationDelay") {
        WebCore::Frame* frame = QWebFramePrivate::core(q->mainFrame());
        if (!frame->tiledBackingStore())
            return;
        int tileCreationDelay = q->property("_q_TiledBackingStoreTileCreationDelay").toInt();
        frame->tiledBackingStore()->setTileCreationDelay(static_cast<double>(tileCreationDelay) / 1000.);
    } else if (event->propertyName() == "_q_TiledBackingStoreKeepAreaMultiplier") {
        WebCore::Frame* frame = QWebFramePrivate::core(q->mainFrame());
        if (!frame->tiledBackingStore())
            return;
        FloatSize keepMultiplier;
        FloatSize coverMultiplier;
        frame->tiledBackingStore()->getKeepAndCoverAreaMultipliers(keepMultiplier, coverMultiplier);
        QSizeF qSize = q->property("_q_TiledBackingStoreKeepAreaMultiplier").toSizeF();
        keepMultiplier = FloatSize(qSize.width(), qSize.height());
        frame->tiledBackingStore()->setKeepAndCoverAreaMultipliers(keepMultiplier, coverMultiplier);
    } else if (event->propertyName() == "_q_TiledBackingStoreCoverAreaMultiplier") {
        WebCore::Frame* frame = QWebFramePrivate::core(q->mainFrame());
        if (!frame->tiledBackingStore())
            return;
        FloatSize keepMultiplier;
        FloatSize coverMultiplier;
        frame->tiledBackingStore()->getKeepAndCoverAreaMultipliers(keepMultiplier, coverMultiplier);
        QSizeF qSize = q->property("_q_TiledBackingStoreCoverAreaMultiplier").toSizeF();
        coverMultiplier = FloatSize(qSize.width(), qSize.height());
        frame->tiledBackingStore()->setKeepAndCoverAreaMultipliers(keepMultiplier, coverMultiplier);
    }
#endif
}
#endif

void QWebPagePrivate::shortcutOverrideEvent(QKeyEvent* event)
{
    WebCore::Frame* frame = page->focusController()->focusedOrMainFrame();
    WebCore::Editor* editor = frame->editor();
    if (editor->canEdit()) {
        if (event->modifiers() == Qt::NoModifier
            || event->modifiers() == Qt::ShiftModifier
            || event->modifiers() == Qt::KeypadModifier) {
                if (event->key() < Qt::Key_Escape) {
                    event->accept();
                } else {
                    switch (event->key()) {
                    case Qt::Key_Return:
                    case Qt::Key_Enter:
                    case Qt::Key_Delete:
                    case Qt::Key_Home:
                    case Qt::Key_End:
                    case Qt::Key_Backspace:
                    case Qt::Key_Left:
                    case Qt::Key_Right:
                    case Qt::Key_Up:
                    case Qt::Key_Down:
                    case Qt::Key_Tab:
                        event->accept();
                    default:
                        break;
                    }
                }
        }
#ifndef QT_NO_SHORTCUT
        else if (editorActionForKeyEvent(event) != QWebPage::NoWebAction)
            event->accept();
#endif
    }
}

bool QWebPagePrivate::handleScrolling(QKeyEvent *ev, Frame *frame)
{
    ScrollDirection direction;
    ScrollGranularity granularity;

#ifndef QT_NO_SHORTCUT
    if (ev == QKeySequence::MoveToNextPage
        || (ev->key() == Qt::Key_Space && !(ev->modifiers() & Qt::ShiftModifier))) {
        granularity = ScrollByPage;
        direction = ScrollDown;
    } else if (ev == QKeySequence::MoveToPreviousPage
               || ((ev->key() == Qt::Key_Space) && (ev->modifiers() & Qt::ShiftModifier))) {
        granularity = ScrollByPage;
        direction = ScrollUp;
    } else
#endif // QT_NO_SHORTCUT
    if ((ev->key() == Qt::Key_Up && ev->modifiers() & Qt::ControlModifier)
               || ev->key() == Qt::Key_Home) {
        granularity = ScrollByDocument;
        direction = ScrollUp;
    } else if ((ev->key() == Qt::Key_Down && ev->modifiers() & Qt::ControlModifier)
               || ev->key() == Qt::Key_End) {
        granularity = ScrollByDocument;
        direction = ScrollDown;
    } else {
        switch (ev->key()) {
            case Qt::Key_Up:
                granularity = ScrollByLine;
                direction = ScrollUp;
                break;
            case Qt::Key_Down:
                granularity = ScrollByLine;
                direction = ScrollDown;
                break;
            case Qt::Key_Left:
                granularity = ScrollByLine;
                direction = ScrollLeft;
                break;
            case Qt::Key_Right:
                granularity = ScrollByLine;
                direction = ScrollRight;
                break;
            default:
                return false;
        }
    }

    return frame->eventHandler()->scrollRecursively(direction, granularity);
}

#if QT_VERSION >= QT_VERSION_CHECK(4, 6, 0)
void QWebPagePrivate::touchEvent(QTouchEvent* event)
{
    WebCore::Frame* frame = QWebFramePrivate::core(mainFrame);
    if (!frame->view())
        return;

    bool accepted = frame->eventHandler()->handleTouchEvent(PlatformTouchEvent(event));
    event->setAccepted(accepted);
}
#endif

/*!
  This method is used by the input method to query a set of properties of the page
  to be able to support complex input method operations as support for surrounding
  text and reconversions.

  \a property specifies which property is queried.

  \sa QWidget::inputMethodEvent(), QInputMethodEvent, QInputContext
*/
QVariant QWebPage::inputMethodQuery(Qt::InputMethodQuery property) const
{
    Frame* frame = d->page->focusController()->focusedFrame();
    if (!frame)
        return QVariant();

    WebCore::Editor* editor = frame->editor();

    RenderObject* renderer = 0;
    RenderTextControl* renderTextControl = 0;

    if (frame->selection()->rootEditableElement())
        renderer = frame->selection()->rootEditableElement()->shadowAncestorNode()->renderer();

    if (renderer && renderer->isTextControl())
        renderTextControl = toRenderTextControl(renderer);

    switch (property) {
        case Qt::ImMicroFocus: {
            WebCore::FrameView* view = frame->view();
            if (view && view->needsLayout()) {
                // We can't access absoluteCaretBounds() while the view needs to layout.
                return QVariant();
            }
            return QVariant(view->contentsToWindow(frame->selection()->absoluteCaretBounds()));
        }
        case Qt::ImFont: {
            if (renderTextControl) {
                RenderStyle* renderStyle = renderTextControl->style();
                return QVariant(QFont(renderStyle->font().font()));
            }
            return QVariant(QFont());
        }
        case Qt::ImCursorPosition: {
            if (renderTextControl) {
                if (editor->hasComposition()) {
                    RefPtr<Range> range = editor->compositionRange();
                    return QVariant(renderTextControl->selectionEnd() - TextIterator::rangeLength(range.get()));
                }
                return QVariant(frame->selection()->extent().offsetInContainerNode());
            }
            return QVariant();
        }
        case Qt::ImSurroundingText: {
            if (renderTextControl) {
                QString text = renderTextControl->text();
                RefPtr<Range> range = editor->compositionRange();
                if (range) {
                    text.remove(range->startPosition().offsetInContainerNode(), TextIterator::rangeLength(range.get()));
                }
                return QVariant(text);
            }
            return QVariant();
        }
        case Qt::ImCurrentSelection: {
            if (renderTextControl) {
                int start = renderTextControl->selectionStart();
                int end = renderTextControl->selectionEnd();
                if (end > start)
                    return QVariant(QString(renderTextControl->text()).mid(start,end-start));
            }
            return QVariant();

        }
#if QT_VERSION >= 0x040600
        case Qt::ImAnchorPosition: {
            if (renderTextControl) {
                if (editor->hasComposition()) {
                    RefPtr<Range> range = editor->compositionRange();
                    return QVariant(renderTextControl->selectionStart() - TextIterator::rangeLength(range.get()));
                }
                return QVariant(frame->selection()->base().offsetInContainerNode());
            }
            return QVariant();
        }
        case Qt::ImMaximumTextLength: {
            if (frame->selection()->isContentEditable()) {
                if (frame->document() && frame->document()->focusedNode()) {
                    if (frame->document()->focusedNode()->hasTagName(HTMLNames::inputTag)) {
                        HTMLInputElement* inputElement = static_cast<HTMLInputElement*>(frame->document()->focusedNode());
                        return QVariant(inputElement->maxLength());
                    }
                }
                return QVariant(InputElement::s_maximumLength);
            }
            return QVariant(0);
        }
#endif
        default:
            return QVariant();
    }
}

/*!
    \internal
*/
void QWebPagePrivate::setInspector(QWebInspector* insp)
{
    if (inspector)
        inspector->d->setFrontend(0);

    if (inspectorIsInternalOnly) {
        QWebInspector* inspToDelete = inspector;
        inspector = 0;
        inspectorIsInternalOnly = false;
        delete inspToDelete;    // Delete after to prevent infinite recursion
    }

    inspector = insp;

    // Give inspector frontend web view if previously created
    if (inspector && inspectorFrontend)
        inspector->d->setFrontend(inspectorFrontend);
}

/*!
    \internal
    Returns the inspector and creates it if it wasn't created yet.
    The instance created here will not be available through QWebPage's API.
*/
QWebInspector* QWebPagePrivate::getOrCreateInspector()
{
#if ENABLE(INSPECTOR)
    if (!inspector) {
        QWebInspector* insp = new QWebInspector;
        insp->setPage(q);
        inspectorIsInternalOnly = true;

        Q_ASSERT(inspector); // Associated through QWebInspector::setPage(q)
    }
#endif
    return inspector;
}

/*! \internal */
InspectorController* QWebPagePrivate::inspectorController()
{
#if ENABLE(INSPECTOR)
    return page->inspectorController();
#else
    return 0;
#endif
}


/*!
   \enum QWebPage::FindFlag

   This enum describes the options available to the findText() function. The options
   can be OR-ed together from the following list:

   \value FindBackward Searches backwards instead of forwards.
   \value FindCaseSensitively By default findText() works case insensitive. Specifying this option
   changes the behaviour to a case sensitive find operation.
   \value FindWrapsAroundDocument Makes findText() restart from the beginning of the document if the end
   was reached and the text was not found.
   \value HighlightAllOccurrences Highlights all existing occurrences of a specific string.
*/

/*!
    \enum QWebPage::LinkDelegationPolicy

    This enum defines the delegation policies a webpage can have when activating links and emitting
    the linkClicked() signal.

    \value DontDelegateLinks No links are delegated. Instead, QWebPage tries to handle them all.
    \value DelegateExternalLinks When activating links that point to documents not stored on the
    local filesystem or an equivalent - such as the Qt resource system - then linkClicked() is emitted.
    \value DelegateAllLinks Whenever a link is activated the linkClicked() signal is emitted.

    \sa QWebPage::linkDelegationPolicy
*/

/*!
    \enum QWebPage::NavigationType

    This enum describes the types of navigation available when browsing through hyperlinked
    documents.

    \value NavigationTypeLinkClicked The user clicked on a link or pressed return on a focused link.
    \value NavigationTypeFormSubmitted The user activated a submit button for an HTML form.
    \value NavigationTypeBackOrForward Navigation to a previously shown document in the back or forward history is requested.
    \value NavigationTypeReload The user activated the reload action.
    \value NavigationTypeFormResubmitted An HTML form was submitted a second time.
    \value NavigationTypeOther A navigation to another document using a method not listed above.

    \sa acceptNavigationRequest()
*/

/*!
    \enum QWebPage::WebAction

    This enum describes the types of action which can be performed on the web page.

    Actions only have an effect when they are applicable. The availability of
    actions can be be determined by checking \l{QAction::}{isEnabled()} on the
    action returned by action().

    One method of enabling the text editing, cursor movement, and text selection actions
    is by setting \l contentEditable to true.

    \value NoWebAction No action is triggered.
    \value OpenLink Open the current link.
    \value OpenLinkInNewWindow Open the current link in a new window.
    \value OpenFrameInNewWindow Replicate the current frame in a new window.
    \value DownloadLinkToDisk Download the current link to the disk.
    \value CopyLinkToClipboard Copy the current link to the clipboard.
    \value OpenImageInNewWindow Open the highlighted image in a new window.
    \value DownloadImageToDisk Download the highlighted image to the disk.
    \value CopyImageToClipboard Copy the highlighted image to the clipboard.
    \value Back Navigate back in the history of navigated links.
    \value Forward Navigate forward in the history of navigated links.
    \value Stop Stop loading the current page.
    \value StopScheduledPageRefresh Stop all pending page refresh/redirect requests.
    \value Reload Reload the current page.
    \value ReloadAndBypassCache Reload the current page, but do not use any local cache. (Added in Qt 4.6)
    \value Cut Cut the content currently selected into the clipboard.
    \value Copy Copy the content currently selected into the clipboard.
    \value Paste Paste content from the clipboard.
    \value Undo Undo the last editing action.
    \value Redo Redo the last editing action.
    \value MoveToNextChar Move the cursor to the next character.
    \value MoveToPreviousChar Move the cursor to the previous character.
    \value MoveToNextWord Move the cursor to the next word.
    \value MoveToPreviousWord Move the cursor to the previous word.
    \value MoveToNextLine Move the cursor to the next line.
    \value MoveToPreviousLine Move the cursor to the previous line.
    \value MoveToStartOfLine Move the cursor to the start of the line.
    \value MoveToEndOfLine Move the cursor to the end of the line.
    \value MoveToStartOfBlock Move the cursor to the start of the block.
    \value MoveToEndOfBlock Move the cursor to the end of the block.
    \value MoveToStartOfDocument Move the cursor to the start of the document.
    \value MoveToEndOfDocument Move the cursor to the end of the document.
    \value SelectNextChar Select to the next character.
    \value SelectPreviousChar Select to the previous character.
    \value SelectNextWord Select to the next word.
    \value SelectPreviousWord Select to the previous word.
    \value SelectNextLine Select to the next line.
    \value SelectPreviousLine Select to the previous line.
    \value SelectStartOfLine Select to the start of the line.
    \value SelectEndOfLine Select to the end of the line.
    \value SelectStartOfBlock Select to the start of the block.
    \value SelectEndOfBlock Select to the end of the block.
    \value SelectStartOfDocument Select to the start of the document.
    \value SelectEndOfDocument Select to the end of the document.
    \value DeleteStartOfWord Delete to the start of the word.
    \value DeleteEndOfWord Delete to the end of the word.
    \value SetTextDirectionDefault Set the text direction to the default direction.
    \value SetTextDirectionLeftToRight Set the text direction to left-to-right.
    \value SetTextDirectionRightToLeft Set the text direction to right-to-left.
    \value ToggleBold Toggle the formatting between bold and normal weight.
    \value ToggleItalic Toggle the formatting between italic and normal style.
    \value ToggleUnderline Toggle underlining.
    \value InspectElement Show the Web Inspector with the currently highlighted HTML element.
    \value InsertParagraphSeparator Insert a new paragraph.
    \value InsertLineSeparator Insert a new line.
    \value SelectAll Selects all content.
    \value PasteAndMatchStyle Paste content from the clipboard with current style.
    \value RemoveFormat Removes formatting and style.
    \value ToggleStrikethrough Toggle the formatting between strikethrough and normal style.
    \value ToggleSubscript Toggle the formatting between subscript and baseline.
    \value ToggleSuperscript Toggle the formatting between supercript and baseline.
    \value InsertUnorderedList Toggles the selection between an ordered list and a normal block.
    \value InsertOrderedList Toggles the selection between an ordered list and a normal block.
    \value Indent Increases the indentation of the currently selected format block by one increment.
    \value Outdent Decreases the indentation of the currently selected format block by one increment.
    \value AlignCenter Applies center alignment to content.
    \value AlignJustified Applies full justification to content.
    \value AlignLeft Applies left justification to content.
    \value AlignRight Applies right justification to content.


    \omitvalue WebActionCount

*/

/*!
    \enum QWebPage::WebWindowType

    This enum describes the types of window that can be created by the createWindow() function.

    \value WebBrowserWindow The window is a regular web browser window.
    \value WebModalDialog The window acts as modal dialog.
*/


/*!
    \class QWebPage::ViewportHints
    \since 4.7
    \brief The QWebPage::ViewportHints class describes hints that can be applied to a viewport.

    QWebPage::ViewportHints provides a description of a viewport, such as viewport geometry,
    initial scale factor with limits, plus information about whether a user should be able
    to scale the contents in the viewport or not, ie. by zooming.

    ViewportHints can be set by a web author using the viewport meta tag extension, documented
    at \l{http://developer.apple.com/safari/library/documentation/appleapplications/reference/safariwebcontent/usingtheviewport/usingtheviewport.html}{Safari Reference Library: Using the Viewport Meta Tag}.

    All values might not be set, as such when dealing with the hints, the developer needs to
    check whether the values are valid. Negative values denote an invalid qreal value.

    \inmodule QtWebKit
*/

/*!
    Constructs an empty QWebPage::ViewportHints.
*/
QWebPage::ViewportHints::ViewportHints()
    : d(0)
    , m_initialScaleFactor(-1.0)
    , m_minimumScaleFactor(-1.0)
    , m_maximumScaleFactor(-1.0)
    , m_isUserScalable(true)
    , m_isValid(false)
{

}

/*!
    Constructs a QWebPage::ViewportHints which is a copy from \a other .
*/
QWebPage::ViewportHints::ViewportHints(const QWebPage::ViewportHints& other)
    : d(other.d)
    , m_initialScaleFactor(other.m_initialScaleFactor)
    , m_minimumScaleFactor(other.m_minimumScaleFactor)
    , m_maximumScaleFactor(other.m_maximumScaleFactor)
    , m_isUserScalable(other.m_isUserScalable)
    , m_isValid(other.m_isValid)
    , m_size(other.m_size)
{

}

/*!
    Destroys the QWebPage::ViewportHints.
*/
QWebPage::ViewportHints::~ViewportHints()
{

}

/*!
    Assigns the given QWebPage::ViewportHints to this viewport hints and returns a
    reference to this.
*/
QWebPage::ViewportHints& QWebPage::ViewportHints::operator=(const QWebPage::ViewportHints& other)
{
    if (this != &other) {
        d = other.d;
        m_initialScaleFactor = other.m_initialScaleFactor;
        m_minimumScaleFactor = other.m_minimumScaleFactor;
        m_maximumScaleFactor = other.m_maximumScaleFactor;
        m_isUserScalable = other.m_isUserScalable;
        m_isValid = other.m_isValid;
        m_size = other.m_size;
    }

    return *this;
}

/*! \fn inline bool QWebPage::ViewportHints::isValid() const
    Returns whether this is a valid ViewportHints or not.

    An invalid ViewportHints will have an empty QSize, negative values for scale factors and
    true for the boolean isUserScalable.
*/

/*! \fn inline QSize QWebPage::ViewportHints::size() const
    Returns the size of the viewport.
*/

/*! \fn inline qreal QWebPage::ViewportHints::initialScaleFactor() const
    Returns the initial scale of the viewport as a multiplier.
*/

/*! \fn inline qreal QWebPage::ViewportHints::minimumScaleFactor() const
    Returns the minimum scale value of the viewport as a multiplier.
*/

/*! \fn inline qreal QWebPage::ViewportHints::maximumScaleFactor() const
    Returns the maximum scale value of the viewport as a multiplier.
*/

/*! \fn inline bool QWebPage::ViewportHints::isUserScalable() const
    Determines whether or not the scale can be modified by the user.
*/


/*!
    \class QWebPage
    \since 4.4
    \brief The QWebPage class provides an object to view and edit web documents.

    \inmodule QtWebKit

    QWebPage holds a main frame responsible for web content, settings, the history
    of navigated links and actions. This class can be used, together with QWebFrame,
    to provide functionality like QWebView in a widget-less environment.

    QWebPage's API is very similar to QWebView, as you are still provided with
    common functions like action() (known as
    \l{QWebView::pageAction()}{pageAction}() in QWebView), triggerAction(),
    findText() and settings(). More QWebView-like functions can be found in the
    main frame of QWebPage, obtained via the mainFrame() function. For example,
    the \l{QWebFrame::load()}{load}(), \l{QWebFrame::setUrl()}{setUrl}() and
    \l{QWebFrame::setHtml()}{setHtml}() functions for QWebPage can be accessed
    using QWebFrame.

    The loadStarted() signal is emitted when the page begins to load.The
    loadProgress() signal, on the other hand, is emitted whenever an element
    of the web page completes loading, such as an embedded image, a script,
    etc. Finally, the loadFinished() signal is emitted when the page has
    loaded completely. Its argument, either true or false, indicates whether
    or not the load operation succeeded.

    \section1 Using QWebPage in a Widget-less Environment

    Before you begin painting a QWebPage object, you need to set the size of
    the viewport by calling setViewportSize(). Then, you invoke the main
    frame's render function (QWebFrame::render()). An example of this
    is shown in the code snippet below.

    Suppose we have a \c Thumbnail class as follows:

    \snippet webkitsnippets/webpage/main.cpp 0

    The \c Thumbnail's constructor takes in a \a url. We connect our QWebPage
    object's \l{QWebPage::}{loadFinished()} signal to our private slot,
    \c render().

    \snippet webkitsnippets/webpage/main.cpp 1

    The \c render() function shows how we can paint a thumbnail using a
    QWebPage object.

    \snippet webkitsnippets/webpage/main.cpp 2

    We begin by setting the \l{QWebPage::viewportSize()}{viewportSize} and
    then we instantiate a QImage object, \c image, with the same size as our
    \l{QWebPage::viewportSize()}{viewportSize}. This image is then sent
    as a parameter to \c painter. Next, we render the contents of the main
    frame and its subframes into \c painter. Finally, we save the scaled image.

    \sa QWebFrame
*/

/*!
    Constructs an empty QWebPage with parent \a parent.
*/
QWebPage::QWebPage(QObject *parent)
    : QObject(parent)
    , d(new QWebPagePrivate(this))
{
    setView(qobject_cast<QWidget*>(parent));

    connect(this, SIGNAL(loadProgress(int)), this, SLOT(_q_onLoadProgressChanged(int)));
#ifndef NDEBUG
    connect(QCoreApplication::instance(), SIGNAL(aboutToQuit()), this, SLOT(_q_cleanupLeakMessages()));
#endif
}

/*!
    Destroys the web page.
*/
QWebPage::~QWebPage()
{
    d->createMainFrame();
    FrameLoader *loader = d->mainFrame->d->frame->loader();
    if (loader)
        loader->detachFromParent();
    if (d->inspector) {
        // Since we have to delete an internal inspector,
        // call setInspector(0) directly to prevent potential crashes
        if (d->inspectorIsInternalOnly)
            d->setInspector(0);
        else
            d->inspector->setPage(0);
    }
    delete d;
}

/*!
    Returns the main frame of the page.

    The main frame provides access to the hierarchy of sub-frames and is also needed if you
    want to explicitly render a web page into a given painter.

    \sa currentFrame()
*/
QWebFrame *QWebPage::mainFrame() const
{
    d->createMainFrame();
    return d->mainFrame;
}

/*!
    Returns the frame currently active.

    \sa mainFrame(), frameCreated()
*/
QWebFrame *QWebPage::currentFrame() const
{
    d->createMainFrame();
    return static_cast<WebCore::FrameLoaderClientQt *>(d->page->focusController()->focusedOrMainFrame()->loader()->client())->webFrame();
}


/*!
    \since 4.6

    Returns the frame at the given point \a pos, or 0 if there is no frame at
    that position.

    \sa mainFrame(), currentFrame()
*/
QWebFrame* QWebPage::frameAt(const QPoint& pos) const
{
    QWebFrame* webFrame = mainFrame();
    if (!webFrame->geometry().contains(pos))
        return 0;
    QWebHitTestResult hitTestResult = webFrame->hitTestContent(pos);
    return hitTestResult.frame();
}

/*!
    Returns a pointer to the view's history of navigated web pages.
*/
QWebHistory *QWebPage::history() const
{
    d->createMainFrame();
    return &d->history;
}

/*!
    Sets the \a view that is associated with the web page.

    \sa view()
*/
void QWebPage::setView(QWidget* view)
{
    if (this->view() == view)
        return;

    d->view = view;
    setViewportSize(view ? view->size() : QSize(0, 0));

    // If we have no client, we install a special client delegating
    // the responsibility to the QWidget. This is the code path
    // handling a.o. the "legacy" QWebView.
    //
    // If such a special delegate already exist, we substitute the view.

    if (d->client) {
        if (d->client->isQWidgetClient())
            static_cast<PageClientQWidget*>(d->client)->view = view;
        return;
    }

    if (view)
        d->client = new PageClientQWidget(view);
}

/*!
    Returns the view widget that is associated with the web page.

    \sa setView()
*/
QWidget *QWebPage::view() const
{
#if QT_VERSION < 0x040600
    return d->view;
#else
    return d->view.data();
#endif
}

/*!
    This function is called whenever a JavaScript program tries to print a \a message to the web browser's console.

    For example in case of evaluation errors the source URL may be provided in \a sourceID as well as the \a lineNumber.

    The default implementation prints nothing.
*/
void QWebPage::javaScriptConsoleMessage(const QString& message, int lineNumber, const QString& sourceID)
{
    Q_UNUSED(sourceID)

    // Catch plugin logDestroy message for LayoutTests/plugins/open-and-close-window-with-plugin.html
    // At this point DRT's WebPage has already been destroyed
    if (QWebPagePrivate::drtRun) {
        if (message == "PLUGIN: NPP_Destroy")
            fprintf (stdout, "CONSOLE MESSAGE: line %d: %s\n", lineNumber, message.toUtf8().constData());
    }
}

/*!
    This function is called whenever a JavaScript program running inside \a frame calls the alert() function with
    the message \a msg.

    The default implementation shows the message, \a msg, with QMessageBox::information.
*/
void QWebPage::javaScriptAlert(QWebFrame *frame, const QString& msg)
{
    Q_UNUSED(frame)
#ifndef QT_NO_MESSAGEBOX
    QWidget* parent = (d->client) ? d->client->ownerWidget() : 0;
    QMessageBox::information(parent, tr("JavaScript Alert - %1").arg(mainFrame()->url().host()), Qt::escape(msg), QMessageBox::Ok);
#endif
}

/*!
    This function is called whenever a JavaScript program running inside \a frame calls the confirm() function
    with the message, \a msg. Returns true if the user confirms the message; otherwise returns false.

    The default implementation executes the query using QMessageBox::information with QMessageBox::Yes and QMessageBox::No buttons.
*/
bool QWebPage::javaScriptConfirm(QWebFrame *frame, const QString& msg)
{
    Q_UNUSED(frame)
#ifdef QT_NO_MESSAGEBOX
    return true;
#else
    QWidget* parent = (d->client) ? d->client->ownerWidget() : 0;
    return QMessageBox::Yes == QMessageBox::information(parent, tr("JavaScript Confirm - %1").arg(mainFrame()->url().host()), Qt::escape(msg), QMessageBox::Yes, QMessageBox::No);
#endif
}

/*!
    This function is called whenever a JavaScript program running inside \a frame tries to prompt the user for input.
    The program may provide an optional message, \a msg, as well as a default value for the input in \a defaultValue.

    If the prompt was cancelled by the user the implementation should return false; otherwise the
    result should be written to \a result and true should be returned. If the prompt was not cancelled by the
    user, the implementation should return true and the result string must not be null.

    The default implementation uses QInputDialog::getText().
*/
bool QWebPage::javaScriptPrompt(QWebFrame *frame, const QString& msg, const QString& defaultValue, QString* result)
{
    Q_UNUSED(frame)
    bool ok = false;
#ifndef QT_NO_INPUTDIALOG
    QWidget* parent = (d->client) ? d->client->ownerWidget() : 0;
    QString x = QInputDialog::getText(parent, tr("JavaScript Prompt - %1").arg(mainFrame()->url().host()), Qt::escape(msg), QLineEdit::Normal, defaultValue, &ok);
    if (ok && result)
        *result = x;
#endif
    return ok;
}

/*!
    \fn bool QWebPage::shouldInterruptJavaScript()
    \since 4.6
    This function is called when a JavaScript program is running for a long period of time.

    If the user wanted to stop the JavaScript the implementation should return true; otherwise false.

    The default implementation executes the query using QMessageBox::information with QMessageBox::Yes and QMessageBox::No buttons.

    \warning Because of binary compatibility constraints, this function is not virtual. If you want to
    provide your own implementation in a QWebPage subclass, reimplement the shouldInterruptJavaScript()
    slot in your subclass instead. QtWebKit will dynamically detect the slot and call it.
*/
bool QWebPage::shouldInterruptJavaScript()
{
#ifdef QT_NO_MESSAGEBOX
    return false;
#else
    QWidget* parent = (d->client) ? d->client->ownerWidget() : 0;
    return QMessageBox::Yes == QMessageBox::information(parent, tr("JavaScript Problem - %1").arg(mainFrame()->url().host()), tr("The script on this page appears to have a problem. Do you want to stop the script?"), QMessageBox::Yes, QMessageBox::No);
#endif
}

/*!
    \fn bool QWebPage::allowGeolocationRequest()
    \since 4.7

    This function is called whenever a JavaScript program running inside \a frame tries to access user location through navigator.geolocation.

    If the user wants to allow access to location then it should return true; otherwise false.

    The default implementation executes the query using QMessageBox::information with QMessageBox::Yes and QMessageBox::No buttons.

    \warning Because of binary compatibility constraints, this function is not virtual. If you want to
    provide your own implementation in a QWebPage subclass, reimplement the allowGeolocationRequest()
    slot in your subclass instead. QtWebKit will dynamically detect the slot and call it.
*/
bool QWebPage::allowGeolocationRequest(QWebFrame *frame)
{
#ifdef QT_NO_MESSAGEBOX
    return false;
#else
    QWidget* parent = (d->client) ? d->client->ownerWidget() : 0;
    return QMessageBox::Yes == QMessageBox::information(parent, tr("Location Request by- %1").arg(frame->url().host()), tr("The page wants to access your location information. Do you want to allow the request?"), QMessageBox::Yes, QMessageBox::No);
#endif
}

/*!
    This function is called whenever WebKit wants to create a new window of the given \a type, for
    example when a JavaScript program requests to open a document in a new window.

    If the new window can be created, the new window's QWebPage is returned; otherwise a null pointer is returned.

    If the view associated with the web page is a QWebView object, then the default implementation forwards
    the request to QWebView's createWindow() function; otherwise it returns a null pointer.

    If \a type is WebModalDialog, the application must call setWindowModality(Qt::ApplicationModal) on the new window.

    \sa acceptNavigationRequest()
*/
QWebPage *QWebPage::createWindow(WebWindowType type)
{
    QWebView *webView = qobject_cast<QWebView*>(view());
    if (webView) {
        QWebView *newView = webView->createWindow(type);
        if (newView)
            return newView->page();
    }
    return 0;
}

/*!
    This function is called whenever WebKit encounters a HTML object element with type "application/x-qt-plugin". It is
    called regardless of the value of QWebSettings::PluginsEnabled. The \a classid, \a url, \a paramNames and \a paramValues
    correspond to the HTML object element attributes and child elements to configure the embeddable object.
*/
QObject *QWebPage::createPlugin(const QString &classid, const QUrl &url, const QStringList &paramNames, const QStringList &paramValues)
{
    Q_UNUSED(classid)
    Q_UNUSED(url)
    Q_UNUSED(paramNames)
    Q_UNUSED(paramValues)
    return 0;
}

static WebCore::FrameLoadRequest frameLoadRequest(const QUrl &url, WebCore::Frame *frame)
{
    WebCore::ResourceRequest rr(url, frame->loader()->outgoingReferrer());
    return WebCore::FrameLoadRequest(rr);
}

static void openNewWindow(const QUrl& url, WebCore::Frame* frame)
{
    if (Page* oldPage = frame->page()) {
        WindowFeatures features;
        if (Page* newPage = oldPage->chrome()->createWindow(frame,
                frameLoadRequest(url, frame), features))
            newPage->chrome()->show();
    }
}

static void collectChildFrames(QWebFrame* frame, QList<QWebFrame*>& list)
{
    list << frame->childFrames();
    QListIterator<QWebFrame*> it(frame->childFrames());
    while (it.hasNext()) {
        collectChildFrames(it.next(), list);
    }
}

/*!
    This function can be called to trigger the specified \a action.
    It is also called by QtWebKit if the user triggers the action, for example
    through a context menu item.

    If \a action is a checkable action then \a checked specified whether the action
    is toggled or not.

    \sa action()
*/
void QWebPage::triggerAction(WebAction action, bool)
{
    WebCore::Frame *frame = d->page->focusController()->focusedOrMainFrame();
    if (!frame)
        return;
    WebCore::Editor *editor = frame->editor();
    const char *command = 0;

    switch (action) {
        case OpenLink:
            if (QWebFrame *targetFrame = d->hitTestResult.linkTargetFrame()) {
                WTF::RefPtr<WebCore::Frame> wcFrame = targetFrame->d->frame;
                targetFrame->d->frame->loader()->loadFrameRequest(frameLoadRequest(d->hitTestResult.linkUrl(), wcFrame.get()),
                                                                  /*lockHistory*/ false, /*lockBackForwardList*/ false, /*event*/ 0,
                                                                  /*FormState*/ 0, SendReferrer);
                break;
            }
            // fall through
        case OpenLinkInNewWindow:
            openNewWindow(d->hitTestResult.linkUrl(), frame);
            break;
        case OpenFrameInNewWindow: {
            KURL url = frame->loader()->documentLoader()->unreachableURL();
            if (url.isEmpty())
                url = frame->loader()->documentLoader()->url();
            openNewWindow(url, frame);
            break;
        }
        case CopyLinkToClipboard: {
#if defined(Q_WS_X11)
            bool oldSelectionMode = Pasteboard::generalPasteboard()->isSelectionMode();
            Pasteboard::generalPasteboard()->setSelectionMode(true);
            editor->copyURL(d->hitTestResult.linkUrl(), d->hitTestResult.linkText());
            Pasteboard::generalPasteboard()->setSelectionMode(oldSelectionMode);
#endif
            editor->copyURL(d->hitTestResult.linkUrl(), d->hitTestResult.linkText());
            break;
        }
        case OpenImageInNewWindow:
            openNewWindow(d->hitTestResult.imageUrl(), frame);
            break;
        case DownloadImageToDisk:
            frame->loader()->client()->startDownload(WebCore::ResourceRequest(d->hitTestResult.imageUrl(), frame->loader()->outgoingReferrer()));
            break;
        case DownloadLinkToDisk:
            frame->loader()->client()->startDownload(WebCore::ResourceRequest(d->hitTestResult.linkUrl(), frame->loader()->outgoingReferrer()));
            break;
#ifndef QT_NO_CLIPBOARD
        case CopyImageToClipboard:
            QApplication::clipboard()->setPixmap(d->hitTestResult.pixmap());
            break;
#endif
        case Back:
            d->page->goBack();
            break;
        case Forward:
            d->page->goForward();
            break;
        case Stop:
            mainFrame()->d->frame->loader()->stopForUserCancel();
            break;
        case Reload:
            mainFrame()->d->frame->loader()->reload(/*endtoendreload*/false);
            break;
        case ReloadAndBypassCache:
            mainFrame()->d->frame->loader()->reload(/*endtoendreload*/true);
            break;
        case SetTextDirectionDefault:
            editor->setBaseWritingDirection(NaturalWritingDirection);
            break;
        case SetTextDirectionLeftToRight:
            editor->setBaseWritingDirection(LeftToRightWritingDirection);
            break;
        case SetTextDirectionRightToLeft:
            editor->setBaseWritingDirection(RightToLeftWritingDirection);
            break;
        case InspectElement: {
#if ENABLE(INSPECTOR)
            if (!d->hitTestResult.isNull()) {
                d->getOrCreateInspector(); // Make sure the inspector is created
                d->inspector->show(); // The inspector is expected to be shown on inspection
                d->page->inspectorController()->inspect(d->hitTestResult.d->innerNonSharedNode.get());
            }
#endif
            break;
        }
        case StopScheduledPageRefresh: {
            QWebFrame* topFrame = mainFrame();
            topFrame->d->frame->redirectScheduler()->cancel();
            QList<QWebFrame*> childFrames;
            collectChildFrames(topFrame, childFrames);
            QListIterator<QWebFrame*> it(childFrames);
            while (it.hasNext())
                it.next()->d->frame->redirectScheduler()->cancel();
            break;
        }
        default:
            command = QWebPagePrivate::editorCommandForWebActions(action);
            break;
    }

    if (command)
        editor->command(command).execute();
}

QSize QWebPage::viewportSize() const
{
    if (d->mainFrame && d->mainFrame->d->frame->view())
        return d->mainFrame->d->frame->view()->frameRect().size();

    return d->viewportSize;
}

/*!
    \property QWebPage::viewportSize
    \brief the size of the viewport

    The size affects for example the visibility of scrollbars
    if the document is larger than the viewport.

    By default, for a newly-created Web page, this property contains a size with
    zero width and height.

    \sa QWebFrame::render(), preferredContentsSize
*/
void QWebPage::setViewportSize(const QSize &size) const
{
    d->viewportSize = size;

    QWebFrame *frame = mainFrame();
    if (frame->d->frame && frame->d->frame->view()) {
        WebCore::FrameView* view = frame->d->frame->view();
        view->setFrameRect(QRect(QPoint(0, 0), size));
        view->adjustViewSize();
    }
}

QSize QWebPage::preferredContentsSize() const
{
    QWebFrame* frame = d->mainFrame;
    if (frame) {
        WebCore::FrameView* view = frame->d->frame->view();
        if (view && view->useFixedLayout())
            return d->mainFrame->d->frame->view()->fixedLayoutSize();
    }

    return d->fixedLayoutSize;
}

/*!
    \property QWebPage::preferredContentsSize
    \since 4.6
    \brief a custom size used for laying out the page contents.

    By default all pages are laid out using the viewport of the page as the base.

    As pages mostly are designed for desktop usage, they often do not layout properly
    on small devices as the contents require a certain view width. For this reason
    it is common to use a different layout size and then scale the contents to fit
    within the actual view.

    If this property is set to a valid size, this size is used for all layout needs
    instead of the size of the viewport.

    Setting an invalid size, makes the page fall back to using the viewport size for layout.

    \sa viewportSize
*/
void QWebPage::setPreferredContentsSize(const QSize& size) const
{
    // FIXME: Rename this method to setCustomLayoutSize

    d->fixedLayoutSize = size;

    QWebFrame* frame = mainFrame();
    if (!frame->d->frame || !frame->d->frame->view())
        return;

    WebCore::FrameView* view = frame->d->frame->view();

    if (size.isValid()) {
        view->setUseFixedLayout(true);
        view->setFixedLayoutSize(size);
    } else if (view->useFixedLayout())
        view->setUseFixedLayout(false);

    if (frame->d->initialLayoutComplete)
        view->layout();
}

/*!
    \fn bool QWebPage::acceptNavigationRequest(QWebFrame *frame, const QNetworkRequest &request, QWebPage::NavigationType type)

    This function is called whenever WebKit requests to navigate \a frame to the resource specified by \a request by means of
    the specified navigation type \a type.

    If \a frame is a null pointer then navigation to a new window is requested. If the request is
    accepted createWindow() will be called.

    The default implementation interprets the page's linkDelegationPolicy and emits linkClicked accordingly or returns true
    to let QWebPage handle the navigation itself.

    \sa createWindow()
*/
bool QWebPage::acceptNavigationRequest(QWebFrame *frame, const QNetworkRequest &request, QWebPage::NavigationType type)
{
    Q_UNUSED(frame)
    if (type == NavigationTypeLinkClicked) {
        switch (d->linkPolicy) {
            case DontDelegateLinks:
                return true;

            case DelegateExternalLinks:
                if (WebCore::SecurityOrigin::shouldTreatURLSchemeAsLocal(request.url().scheme()))
                    return true;
                emit linkClicked(request.url());
                return false;

            case DelegateAllLinks:
                emit linkClicked(request.url());
                return false;
        }
    }
    return true;
}

/*!
    \property QWebPage::selectedText
    \brief the text currently selected

    By default, this property contains an empty string.

    \sa selectionChanged()
*/
QString QWebPage::selectedText() const
{
    d->createMainFrame();
    return d->page->focusController()->focusedOrMainFrame()->selectedText();
}

#ifndef QT_NO_ACTION
/*!
   Returns a QAction for the specified WebAction \a action.

   The action is owned by the QWebPage but you can customize the look by
   changing its properties.

   QWebPage also takes care of implementing the action, so that upon
   triggering the corresponding action is performed on the page.

   \sa triggerAction()
*/
QAction *QWebPage::action(WebAction action) const
{
    if (action == QWebPage::NoWebAction) return 0;
    if (d->actions[action])
        return d->actions[action];

    QString text;
    QIcon icon;
    QStyle *style = d->client ? d->client->style() : qApp->style();
    bool checkable = false;

    switch (action) {
        case OpenLink:
            text = contextMenuItemTagOpenLink();
            break;
        case OpenLinkInNewWindow:
            text = contextMenuItemTagOpenLinkInNewWindow();
            break;
        case OpenFrameInNewWindow:
            text = contextMenuItemTagOpenFrameInNewWindow();
            break;

        case DownloadLinkToDisk:
            text = contextMenuItemTagDownloadLinkToDisk();
            break;
        case CopyLinkToClipboard:
            text = contextMenuItemTagCopyLinkToClipboard();
            break;

        case OpenImageInNewWindow:
            text = contextMenuItemTagOpenImageInNewWindow();
            break;
        case DownloadImageToDisk:
            text = contextMenuItemTagDownloadImageToDisk();
            break;
        case CopyImageToClipboard:
            text = contextMenuItemTagCopyImageToClipboard();
            break;

        case Back:
            text = contextMenuItemTagGoBack();
            icon = style->standardIcon(QStyle::SP_ArrowBack);
            break;
        case Forward:
            text = contextMenuItemTagGoForward();
            icon = style->standardIcon(QStyle::SP_ArrowForward);
            break;
        case Stop:
            text = contextMenuItemTagStop();
            icon = style->standardIcon(QStyle::SP_BrowserStop);
            break;
        case Reload:
            text = contextMenuItemTagReload();
            icon = style->standardIcon(QStyle::SP_BrowserReload);
            break;

        case Cut:
            text = contextMenuItemTagCut();
            break;
        case Copy:
            text = contextMenuItemTagCopy();
            break;
        case Paste:
            text = contextMenuItemTagPaste();
            break;
#ifndef QT_NO_UNDOSTACK
        case Undo: {
            QAction *a = undoStack()->createUndoAction(d->q);
            d->actions[action] = a;
            return a;
        }
        case Redo: {
            QAction *a = undoStack()->createRedoAction(d->q);
            d->actions[action] = a;
            return a;
        }
#endif // QT_NO_UNDOSTACK
        case MoveToNextChar:
            text = tr("Move the cursor to the next character");
            break;
        case MoveToPreviousChar:
            text = tr("Move the cursor to the previous character");
            break;
        case MoveToNextWord:
            text = tr("Move the cursor to the next word");
            break;
        case MoveToPreviousWord:
            text = tr("Move the cursor to the previous word");
            break;
        case MoveToNextLine:
            text = tr("Move the cursor to the next line");
            break;
        case MoveToPreviousLine:
            text = tr("Move the cursor to the previous line");
            break;
        case MoveToStartOfLine:
            text = tr("Move the cursor to the start of the line");
            break;
        case MoveToEndOfLine:
            text = tr("Move the cursor to the end of the line");
            break;
        case MoveToStartOfBlock:
            text = tr("Move the cursor to the start of the block");
            break;
        case MoveToEndOfBlock:
            text = tr("Move the cursor to the end of the block");
            break;
        case MoveToStartOfDocument:
            text = tr("Move the cursor to the start of the document");
            break;
        case MoveToEndOfDocument:
            text = tr("Move the cursor to the end of the document");
            break;
        case SelectAll:
            text = tr("Select all");
            break;
        case SelectNextChar:
            text = tr("Select to the next character");
            break;
        case SelectPreviousChar:
            text = tr("Select to the previous character");
            break;
        case SelectNextWord:
            text = tr("Select to the next word");
            break;
        case SelectPreviousWord:
            text = tr("Select to the previous word");
            break;
        case SelectNextLine:
            text = tr("Select to the next line");
            break;
        case SelectPreviousLine:
            text = tr("Select to the previous line");
            break;
        case SelectStartOfLine:
            text = tr("Select to the start of the line");
            break;
        case SelectEndOfLine:
            text = tr("Select to the end of the line");
            break;
        case SelectStartOfBlock:
            text = tr("Select to the start of the block");
            break;
        case SelectEndOfBlock:
            text = tr("Select to the end of the block");
            break;
        case SelectStartOfDocument:
            text = tr("Select to the start of the document");
            break;
        case SelectEndOfDocument:
            text = tr("Select to the end of the document");
            break;
        case DeleteStartOfWord:
            text = tr("Delete to the start of the word");
            break;
        case DeleteEndOfWord:
            text = tr("Delete to the end of the word");
            break;

        case SetTextDirectionDefault:
            text = contextMenuItemTagDefaultDirection();
            break;
        case SetTextDirectionLeftToRight:
            text = contextMenuItemTagLeftToRight();
            checkable = true;
            break;
        case SetTextDirectionRightToLeft:
            text = contextMenuItemTagRightToLeft();
            checkable = true;
            break;

        case ToggleBold:
            text = contextMenuItemTagBold();
            checkable = true;
            break;
        case ToggleItalic:
            text = contextMenuItemTagItalic();
            checkable = true;
            break;
        case ToggleUnderline:
            text = contextMenuItemTagUnderline();
            checkable = true;
            break;

        case InspectElement:
            text = contextMenuItemTagInspectElement();
            break;

        case InsertParagraphSeparator:
            text = tr("Insert a new paragraph");
            break;
        case InsertLineSeparator:
            text = tr("Insert a new line");
            break;

        case PasteAndMatchStyle:
            text = tr("Paste and Match Style");
            break;
        case RemoveFormat:
            text = tr("Remove formatting");
            break;

        case ToggleStrikethrough:
            text = tr("Strikethrough");
            checkable = true;
            break;
        case ToggleSubscript:
            text = tr("Subscript");
            checkable = true;
            break;
        case ToggleSuperscript:
            text = tr("Superscript");
            checkable = true;
            break;
        case InsertUnorderedList:
            text = tr("Insert Bulleted List");
            checkable = true;
            break;
        case InsertOrderedList:
            text = tr("Insert Numbered List");
            checkable = true;
            break;
        case Indent:
            text = tr("Indent");
            break;
        case Outdent:
            text = tr("Outdent");
            break;
        case AlignCenter:
            text = tr("Center");
            break;
        case AlignJustified:
            text = tr("Justify");
            break;
        case AlignLeft:
            text = tr("Align Left");
            break;
        case AlignRight:
            text = tr("Align Right");
            break;

        case NoWebAction:
            return 0;
    }

    if (text.isEmpty())
        return 0;

    QAction *a = new QAction(d->q);
    a->setText(text);
    a->setData(action);
    a->setCheckable(checkable);
    a->setIcon(icon);

    connect(a, SIGNAL(triggered(bool)),
            this, SLOT(_q_webActionTriggered(bool)));

    d->actions[action] = a;
    d->updateAction(action);
    return a;
}
#endif // QT_NO_ACTION

/*!
    \property QWebPage::modified
    \brief whether the page contains unsubmitted form data, or the contents have been changed.

    By default, this property is false.

    \sa contentsChanged(), contentEditable, undoStack()
*/
bool QWebPage::isModified() const
{
#ifdef QT_NO_UNDOSTACK
    return false;
#else
    if (!d->undoStack)
        return false;
    return d->undoStack->canUndo();
#endif // QT_NO_UNDOSTACK
}

#ifndef QT_NO_UNDOSTACK
/*!
    Returns a pointer to the undo stack used for editable content.

    \sa modified
*/
QUndoStack *QWebPage::undoStack() const
{
    if (!d->undoStack)
        d->undoStack = new QUndoStack(const_cast<QWebPage *>(this));

    return d->undoStack;
}
#endif // QT_NO_UNDOSTACK

/*! \reimp
*/
bool QWebPage::event(QEvent *ev)
{
    switch (ev->type()) {
    case QEvent::Timer:
        d->timerEvent(static_cast<QTimerEvent*>(ev));
        break;
    case QEvent::MouseMove:
        d->mouseMoveEvent(static_cast<QMouseEvent*>(ev));
        break;
    case QEvent::GraphicsSceneMouseMove:
        d->mouseMoveEvent(static_cast<QGraphicsSceneMouseEvent*>(ev));
        break;
    case QEvent::MouseButtonPress:
        d->mousePressEvent(static_cast<QMouseEvent*>(ev));
        break;
    case QEvent::GraphicsSceneMousePress:
        d->mousePressEvent(static_cast<QGraphicsSceneMouseEvent*>(ev));
        break;
    case QEvent::MouseButtonDblClick:
        d->mouseDoubleClickEvent(static_cast<QMouseEvent*>(ev));
        break;
    case QEvent::GraphicsSceneMouseDoubleClick:
        d->mouseDoubleClickEvent(static_cast<QGraphicsSceneMouseEvent*>(ev));
        break;
    case QEvent::MouseButtonRelease:
        d->mouseReleaseEvent(static_cast<QMouseEvent*>(ev));
        break;
    case QEvent::GraphicsSceneMouseRelease:
        d->mouseReleaseEvent(static_cast<QGraphicsSceneMouseEvent*>(ev));
        break;
#ifndef QT_NO_CONTEXTMENU
    case QEvent::ContextMenu:
        d->contextMenuEvent(static_cast<QContextMenuEvent*>(ev)->globalPos());
        break;
    case QEvent::GraphicsSceneContextMenu:
        d->contextMenuEvent(static_cast<QGraphicsSceneContextMenuEvent*>(ev)->screenPos());
        break;
#endif
#ifndef QT_NO_WHEELEVENT
    case QEvent::Wheel:
        d->wheelEvent(static_cast<QWheelEvent*>(ev));
        break;
    case QEvent::GraphicsSceneWheel:
        d->wheelEvent(static_cast<QGraphicsSceneWheelEvent*>(ev));
        break;
#endif
    case QEvent::KeyPress:
        d->keyPressEvent(static_cast<QKeyEvent*>(ev));
        break;
    case QEvent::KeyRelease:
        d->keyReleaseEvent(static_cast<QKeyEvent*>(ev));
        break;
    case QEvent::FocusIn:
        d->focusInEvent(static_cast<QFocusEvent*>(ev));
        break;
    case QEvent::FocusOut:
        d->focusOutEvent(static_cast<QFocusEvent*>(ev));
        break;
#ifndef QT_NO_DRAGANDDROP
    case QEvent::DragEnter:
        d->dragEnterEvent(static_cast<QDragEnterEvent*>(ev));
        break;
    case QEvent::GraphicsSceneDragEnter:
        d->dragEnterEvent(static_cast<QGraphicsSceneDragDropEvent*>(ev));
        break;
    case QEvent::DragLeave:
        d->dragLeaveEvent(static_cast<QDragLeaveEvent*>(ev));
        break;
    case QEvent::GraphicsSceneDragLeave:
        d->dragLeaveEvent(static_cast<QGraphicsSceneDragDropEvent*>(ev));
        break;
    case QEvent::DragMove:
        d->dragMoveEvent(static_cast<QDragMoveEvent*>(ev));
        break;
    case QEvent::GraphicsSceneDragMove:
        d->dragMoveEvent(static_cast<QGraphicsSceneDragDropEvent*>(ev));
        break;
    case QEvent::Drop:
        d->dropEvent(static_cast<QDropEvent*>(ev));
        break;
    case QEvent::GraphicsSceneDrop:
        d->dropEvent(static_cast<QGraphicsSceneDragDropEvent*>(ev));
        break;
#endif
    case QEvent::InputMethod:
        d->inputMethodEvent(static_cast<QInputMethodEvent*>(ev));
    case QEvent::ShortcutOverride:
        d->shortcutOverrideEvent(static_cast<QKeyEvent*>(ev));
        break;
    case QEvent::Leave:
        d->leaveEvent(ev);
        break;
#if QT_VERSION >= QT_VERSION_CHECK(4, 6, 0)
    case QEvent::TouchBegin:
    case QEvent::TouchUpdate:
    case QEvent::TouchEnd:
        d->touchEvent(static_cast<QTouchEvent*>(ev));
        break;
#endif
#ifndef QT_NO_PROPERTIES
    case QEvent::DynamicPropertyChange:
        d->dynamicPropertyChangeEvent(static_cast<QDynamicPropertyChangeEvent*>(ev));
        break;
#endif
    default:
        return QObject::event(ev);
    }

    return true;
}

/*!
    Similar to QWidget::focusNextPrevChild() it focuses the next focusable web element
    if \a next is true; otherwise the previous element is focused.

    Returns true if it can find a new focusable element, or false if it can't.
*/
bool QWebPage::focusNextPrevChild(bool next)
{
    QKeyEvent ev(QEvent::KeyPress, Qt::Key_Tab, Qt::KeyboardModifiers(next ? Qt::NoModifier : Qt::ShiftModifier));
    d->keyPressEvent(&ev);
    bool hasFocusedNode = false;
    Frame *frame = d->page->focusController()->focusedFrame();
    if (frame) {
        Document *document = frame->document();
        hasFocusedNode = document && document->focusedNode();
    }
    //qDebug() << "focusNextPrevChild(" << next << ") =" << ev.isAccepted() << "focusedNode?" << hasFocusedNode;
    return hasFocusedNode;
}

/*!
    \property QWebPage::contentEditable
    \brief whether the content in this QWebPage is editable or not
    \since 4.5

    If this property is enabled the contents of the page can be edited by the user through a visible
    cursor. If disabled (the default) only HTML elements in the web page with their
    \c{contenteditable} attribute set are editable.

    \sa modified, contentsChanged(), WebAction
*/
void QWebPage::setContentEditable(bool editable)
{
    if (d->editable != editable) {
        d->editable = editable;
        d->page->setTabKeyCyclesThroughElements(!editable);
        if (d->mainFrame) {
            WebCore::Frame* frame = d->mainFrame->d->frame;
            if (editable) {
                frame->applyEditingStyleToBodyElement();
                // FIXME: mac port calls this if there is no selectedDOMRange
                //frame->setSelectionFromNone();
            }
        }

        d->updateEditorActions();
    }
}

bool QWebPage::isContentEditable() const
{
    return d->editable;
}

/*!
    \property QWebPage::forwardUnsupportedContent
    \brief whether QWebPage should forward unsupported content

    If enabled, the unsupportedContent() signal is emitted with a network reply that
    can be used to read the content.

    If disabled, the download of such content is aborted immediately.

    By default unsupported content is not forwarded.
*/

void QWebPage::setForwardUnsupportedContent(bool forward)
{
    d->forwardUnsupportedContent = forward;
}

bool QWebPage::forwardUnsupportedContent() const
{
    return d->forwardUnsupportedContent;
}

/*!
    \property QWebPage::linkDelegationPolicy
    \brief how QWebPage should delegate the handling of links through the
    linkClicked() signal

    The default is to delegate no links.
*/

void QWebPage::setLinkDelegationPolicy(LinkDelegationPolicy policy)
{
    d->linkPolicy = policy;
}

QWebPage::LinkDelegationPolicy QWebPage::linkDelegationPolicy() const
{
    return d->linkPolicy;
}

#ifndef QT_NO_CONTEXTMENU
/*!
    Filters the context menu event, \a event, through handlers for scrollbars and
    custom event handlers in the web page. Returns true if the event was handled;
    otherwise false.

    A web page may swallow a context menu event through a custom event handler, allowing for context
    menus to be implemented in HTML/JavaScript. This is used by \l{http://maps.google.com/}{Google
    Maps}, for example.
*/
bool QWebPage::swallowContextMenuEvent(QContextMenuEvent *event)
{
    d->page->contextMenuController()->clearContextMenu();

    if (QWebFrame* webFrame = frameAt(event->pos())) {
        Frame* frame = QWebFramePrivate::core(webFrame);
        if (Scrollbar* scrollbar = frame->view()->scrollbarAtPoint(PlatformMouseEvent(event, 1).pos()))
            return scrollbar->contextMenu(PlatformMouseEvent(event, 1));
    }

    WebCore::Frame* focusedFrame = d->page->focusController()->focusedOrMainFrame();
    focusedFrame->eventHandler()->sendContextMenuEvent(PlatformMouseEvent(event, 1));
    ContextMenu *menu = d->page->contextMenuController()->contextMenu();
    // If the website defines its own handler then sendContextMenuEvent takes care of
    // calling/showing it and the context menu pointer will be zero. This is the case
    // on maps.google.com for example.

    return !menu;
}
#endif // QT_NO_CONTEXTMENU

/*!
    Updates the page's actions depending on the position \a pos. For example if \a pos is over an image
    element the CopyImageToClipboard action is enabled.
*/
void QWebPage::updatePositionDependentActions(const QPoint &pos)
{
#ifndef QT_NO_ACTION
    // First we disable all actions, but keep track of which ones were originally enabled.
    QBitArray originallyEnabledWebActions(QWebPage::WebActionCount);
    for (int i = ContextMenuItemTagNoAction; i < ContextMenuItemBaseApplicationTag; ++i) {
        QWebPage::WebAction action = webActionForContextMenuAction(WebCore::ContextMenuAction(i));
        if (QAction *a = this->action(action)) {
            originallyEnabledWebActions.setBit(action, a->isEnabled());
            a->setEnabled(false);
        }
    }
#endif // QT_NO_ACTION

    d->createMainFrame();
    WebCore::Frame* focusedFrame = d->page->focusController()->focusedOrMainFrame();
    HitTestResult result = focusedFrame->eventHandler()->hitTestResultAtPoint(focusedFrame->view()->windowToContents(pos), /*allowShadowContent*/ false);

    if (result.scrollbar())
        d->hitTestResult = QWebHitTestResult();
    else
        d->hitTestResult = QWebHitTestResult(new QWebHitTestResultPrivate(result));
    WebCore::ContextMenu menu(result);
    menu.populate();
    
#if ENABLE(INSPECTOR)
    if (d->page->inspectorController()->enabled())
        menu.addInspectElementItem();
#endif

    QBitArray visitedWebActions(QWebPage::WebActionCount);

#ifndef QT_NO_CONTEXTMENU
    delete d->currentContextMenu;

    // Then we let createContextMenu() enable the actions that are put into the menu
    d->currentContextMenu = d->createContextMenu(&menu, menu.platformDescription(), &visitedWebActions);
#endif // QT_NO_CONTEXTMENU

#ifndef QT_NO_ACTION
    // Finally, we restore the original enablement for the actions that were not put into the menu.
    originallyEnabledWebActions &= ~visitedWebActions; // Mask out visited actions (they're part of the menu)
    for (int i = 0; i < QWebPage::WebActionCount; ++i) {
        if (originallyEnabledWebActions.at(i)) {
            if (QAction *a = this->action(QWebPage::WebAction(i)))
                a->setEnabled(true);
        }
    }
#endif // QT_NO_ACTION

    // This whole process ensures that any actions put into to the context menu has the right
    // enablement, while also keeping the correct enablement for actions that were left out of
    // the menu.

}



/*!
    \enum QWebPage::Extension

    This enum describes the types of extensions that the page can support. Before using these extensions, you
    should verify that the extension is supported by calling supportsExtension().

    \value ChooseMultipleFilesExtension Whether the web page supports multiple file selection.
    This extension is invoked when the web content requests one or more file names, for example
    as a result of the user clicking on a "file upload" button in a HTML form where multiple
    file selection is allowed.

    \value ErrorPageExtension Whether the web page can provide an error page when loading fails.
    (introduced in Qt 4.6)

    \sa ChooseMultipleFilesExtensionOption, ChooseMultipleFilesExtensionReturn, ErrorPageExtensionOption, ErrorPageExtensionReturn
*/

/*!
    \enum QWebPage::ErrorDomain
    \since 4.6

    This enum describes the domain of an ErrorPageExtensionOption object (i.e. the layer in which the error occurred).

    \value QtNetwork The error occurred in the QtNetwork layer; the error code is of type QNetworkReply::NetworkError.
    \value Http The error occurred in the HTTP layer; the error code is a HTTP status code (see QNetworkRequest::HttpStatusCodeAttribute).
    \value WebKit The error is an internal WebKit error.
*/

/*!
    \class QWebPage::ExtensionOption
    \since 4.4
    \brief The ExtensionOption class provides an extended input argument to QWebPage's extension support.

    \inmodule QtWebKit

    \sa QWebPage::extension() QWebPage::ExtensionReturn
*/


/*!
    \class QWebPage::ExtensionReturn
    \since 4.4
    \brief The ExtensionReturn class provides an output result from a QWebPage's extension.

    \inmodule QtWebKit

    \sa QWebPage::extension() QWebPage::ExtensionOption
*/

/*!
    \class QWebPage::ErrorPageExtensionOption
    \since 4.6
    \brief The ErrorPageExtensionOption class describes the option
    for the error page extension.

    \inmodule QtWebKit

    The ErrorPageExtensionOption class holds the \a url for which an error occurred as well as
    the associated \a frame.

    The error itself is reported by an error \a domain, the \a error code as well as \a errorString.

    \sa QWebPage::extension() QWebPage::ErrorPageExtensionReturn
*/

/*!
    \variable QWebPage::ErrorPageExtensionOption::url
    \brief the url for which an error occurred
*/

/*!
    \variable QWebPage::ErrorPageExtensionOption::frame
    \brief the frame associated with the error
*/

/*!
    \variable QWebPage::ErrorPageExtensionOption::domain
    \brief the domain that reported the error
*/

/*!
    \variable QWebPage::ErrorPageExtensionOption::error
    \brief the error code. Interpretation of the value depends on the \a domain
    \sa QWebPage::ErrorDomain
*/

/*!
    \variable QWebPage::ErrorPageExtensionOption::errorString
    \brief a string that describes the error
*/

/*!
    \class QWebPage::ErrorPageExtensionReturn
    \since 4.6
    \brief The ErrorPageExtensionReturn describes the error page, which will be shown for the
    frame for which the error occured.

    \inmodule QtWebKit

    The ErrorPageExtensionReturn class holds the data needed for creating an error page. Some are
    optional such as \a contentType, which defaults to "text/html", as well as the \a encoding, which
    is assumed to be UTF-8 if not indicated otherwise.

    The error page is stored in the \a content byte array, as HTML content. In order to convert a
    QString to a byte array, the QString::toUtf8() method can be used.

    External objects such as stylesheets or images referenced in the HTML are located relative to
    \a baseUrl.

    \sa QWebPage::extension() QWebPage::ErrorPageExtensionOption, QString::toUtf8()
*/

/*!
    \fn QWebPage::ErrorPageExtensionReturn::ErrorPageExtensionReturn()

    Constructs a new error page object.
*/


/*!
    \variable QWebPage::ErrorPageExtensionReturn::contentType
    \brief the error page's content type
*/

/*!
    \variable QWebPage::ErrorPageExtensionReturn::encoding
    \brief the error page encoding
*/

/*!
    \variable QWebPage::ErrorPageExtensionReturn::baseUrl
    \brief the base url

    External objects such as stylesheets or images referenced in the HTML are located relative to this url.
*/

/*!
    \variable QWebPage::ErrorPageExtensionReturn::content
    \brief the HTML content of the error page
*/

/*!
    \class QWebPage::ChooseMultipleFilesExtensionOption
    \since 4.5
    \brief The ChooseMultipleFilesExtensionOption class describes the option
    for the multiple files selection extension.

    \inmodule QtWebKit

    The ChooseMultipleFilesExtensionOption class holds the frame originating the request
    and the suggested filenames which might be provided.

    \sa QWebPage::extension() QWebPage::chooseFile(), QWebPage::ChooseMultipleFilesExtensionReturn
*/

/*!
    \variable QWebPage::ChooseMultipleFilesExtensionOption::parentFrame
    \brief The frame in which the request originated
*/

/*!
    \variable QWebPage::ChooseMultipleFilesExtensionOption::suggestedFileNames
    \brief The suggested filenames
*/

/*!
    \variable QWebPage::ChooseMultipleFilesExtensionReturn::fileNames
    \brief The selected filenames
*/

/*!
    \class QWebPage::ChooseMultipleFilesExtensionReturn
    \since 4.5
    \brief The ChooseMultipleFilesExtensionReturn describes the return value
    for the multiple files selection extension.

    \inmodule QtWebKit

    The ChooseMultipleFilesExtensionReturn class holds the filenames selected by the user
    when the extension is invoked.

    \sa QWebPage::extension() QWebPage::ChooseMultipleFilesExtensionOption
*/

/*!
    This virtual function can be reimplemented in a QWebPage subclass to provide support for extensions. The \a option
    argument is provided as input to the extension; the output results can be stored in \a output.

    The behavior of this function is determined by \a extension. The \a option
    and \a output values are typically casted to the corresponding types (for
    example, ChooseMultipleFilesExtensionOption and
    ChooseMultipleFilesExtensionReturn for ChooseMultipleFilesExtension).

    You can call supportsExtension() to check if an extension is supported by the page.

    Returns true if the extension was called successfully; otherwise returns false.

    \sa supportsExtension(), Extension
*/
bool QWebPage::extension(Extension extension, const ExtensionOption *option, ExtensionReturn *output)
{
#ifndef QT_NO_FILEDIALOG
    if (extension == ChooseMultipleFilesExtension) {
        // FIXME: do not ignore suggestedFiles
        QStringList suggestedFiles = static_cast<const ChooseMultipleFilesExtensionOption*>(option)->suggestedFileNames;
        QWidget* parent = (d->client) ? d->client->ownerWidget() : 0;
        QStringList names = QFileDialog::getOpenFileNames(parent, QString::null);
        static_cast<ChooseMultipleFilesExtensionReturn*>(output)->fileNames = names;
        return true;
    }
#endif

    return false;
}

/*!
    This virtual function returns true if the web page supports \a extension; otherwise false is returned.

    \sa extension()
*/
bool QWebPage::supportsExtension(Extension extension) const
{
#ifndef QT_NO_FILEDIALOG
    return extension == ChooseMultipleFilesExtension;
#else
    Q_UNUSED(extension);
    return false;
#endif
}

/*!
    Finds the specified string, \a subString, in the page, using the given \a options.

    If the HighlightAllOccurrences flag is passed, the function will highlight all occurrences
    that exist in the page. All subsequent calls will extend the highlight, rather than
    replace it, with occurrences of the new string.

    If the HighlightAllOccurrences flag is not passed, the function will select an occurrence
    and all subsequent calls will replace the current occurrence with the next one.

    To clear the selection, just pass an empty string.

    Returns true if \a subString was found; otherwise returns false.
*/
bool QWebPage::findText(const QString &subString, FindFlags options)
{
    ::TextCaseSensitivity caseSensitivity = ::TextCaseInsensitive;
    if (options & FindCaseSensitively)
        caseSensitivity = ::TextCaseSensitive;

    if (options & HighlightAllOccurrences) {
        if (subString.isEmpty()) {
            d->page->unmarkAllTextMatches();
            return true;
        } else
            return d->page->markAllMatchesForText(subString, caseSensitivity, true, 0);
    } else {
        if (subString.isEmpty()) {
            d->page->mainFrame()->selection()->clear();
            Frame* frame = d->page->mainFrame()->tree()->firstChild();
            while (frame) {
                frame->selection()->clear();
                frame = frame->tree()->traverseNextWithWrap(false);
            }
        }
        ::FindDirection direction = ::FindDirectionForward;
        if (options & FindBackward)
            direction = ::FindDirectionBackward;

        const bool shouldWrap = options & FindWrapsAroundDocument;

        return d->page->findString(subString, caseSensitivity, direction, shouldWrap);
    }
}

/*!
    Returns a pointer to the page's settings object.

    \sa QWebSettings::globalSettings()
*/
QWebSettings *QWebPage::settings() const
{
    return d->settings;
}

/*!
    This function is called when the web content requests a file name, for example
    as a result of the user clicking on a "file upload" button in a HTML form.

    A suggested filename may be provided in \a suggestedFile. The frame originating the
    request is provided as \a parentFrame.

    \sa ChooseMultipleFilesExtension
*/
QString QWebPage::chooseFile(QWebFrame *parentFrame, const QString& suggestedFile)
{
    Q_UNUSED(parentFrame)
#ifndef QT_NO_FILEDIALOG
    QWidget* parent = (d->client) ? d->client->ownerWidget() : 0;
    return QFileDialog::getOpenFileName(parent, QString::null, suggestedFile);
#else
    return QString::null;
#endif
}

/*!
    Sets the QNetworkAccessManager \a manager responsible for serving network requests for this
    QWebPage.

    \note It is currently not supported to change the network access manager after the
    QWebPage has used it. The results of doing this are undefined.

    \sa networkAccessManager()
*/
void QWebPage::setNetworkAccessManager(QNetworkAccessManager *manager)
{
    if (manager == d->networkManager)
        return;
    if (d->networkManager && d->networkManager->parent() == this)
        delete d->networkManager;
    d->networkManager = manager;
}

/*!
    Returns the QNetworkAccessManager that is responsible for serving network
    requests for this QWebPage.

    \sa setNetworkAccessManager()
*/
QNetworkAccessManager *QWebPage::networkAccessManager() const
{
    if (!d->networkManager) {
        QWebPage *that = const_cast<QWebPage *>(this);
        that->d->networkManager = new QNetworkAccessManager(that);
    }
    return d->networkManager;
}

/*!
    Sets the QWebPluginFactory \a factory responsible for creating plugins embedded into this
    QWebPage.

    Note: The plugin factory is only used if the QWebSettings::PluginsEnabled attribute is enabled.

    \sa pluginFactory()
*/
void QWebPage::setPluginFactory(QWebPluginFactory *factory)
{
    d->pluginFactory = factory;
}

/*!
    Returns the QWebPluginFactory that is responsible for creating plugins embedded into
    this QWebPage. If no plugin factory is installed a null pointer is returned.

    \sa setPluginFactory()
*/
QWebPluginFactory *QWebPage::pluginFactory() const
{
    return d->pluginFactory;
}

/*!
    This function is called when a user agent for HTTP requests is needed. You can reimplement this
    function to dynamically return different user agents for different URLs, based on the \a url parameter.

    The default implementation returns the following value:

    "Mozilla/5.0 (%Platform%; %Security%; %Subplatform%; %Locale%) AppleWebKit/%WebKitVersion% (KHTML, like Gecko) %AppVersion Safari/%WebKitVersion%"

    On mobile platforms such as Symbian S60 and Maemo, "Mobile Safari" is used instead of "Safari".

    In this string the following values are replaced at run-time:
    \list
    \o %Platform% and %Subplatform% are expanded to the windowing system and the operation system.
    \o %Security% expands to U if SSL is enabled, otherwise N. SSL is enabled if QSslSocket::supportsSsl() returns true.
    \o %Locale% is replaced with QLocale::name(). The locale is determined from the view of the QWebPage. If no view is set on the QWebPage,
    then a default constructed QLocale is used instead.
    \o %WebKitVersion% is the version of WebKit the application was compiled against.
    \o %AppVersion% expands to QCoreApplication::applicationName()/QCoreApplication::applicationVersion() if they're set; otherwise defaulting to Qt and the current Qt version.
    \endlist
*/
QString QWebPage::userAgentForUrl(const QUrl&) const
{
    // splitting the string in three and user QStringBuilder is better than using QString::arg()
    static QString firstPart;
    static QString secondPart;
    static QString thirdPart;

    if (firstPart.isNull() || secondPart.isNull() || thirdPart.isNull()) {
        QString firstPartTemp;
        firstPartTemp.reserve(150);
        firstPartTemp += QString::fromLatin1("Mozilla/5.0 ("

    // Platform
#ifdef Q_WS_MAC
        "Macintosh"
#elif defined Q_WS_QWS
        "QtEmbedded"
#elif defined Q_WS_WIN
        "Windows"
#elif defined Q_WS_X11
        "X11"
#elif defined Q_OS_SYMBIAN
        "Symbian"
#else
        "Unknown"
#endif
    );

#if defined Q_OS_SYMBIAN
        QSysInfo::SymbianVersion symbianVersion = QSysInfo::symbianVersion();
        switch (symbianVersion) {
        case QSysInfo::SV_9_2:
            firstPartTemp += QString::fromLatin1("OS/9.2");
            break;
        case QSysInfo::SV_9_3:
            firstPartTemp += QString::fromLatin1("OS/9.3");
            break;                
        case QSysInfo::SV_9_4:
            firstPartTemp += QString::fromLatin1("OS/9.4");
            break;
        case QSysInfo::SV_SF_2:
            firstPartTemp += QString::fromLatin1("/2");
            break;
        case QSysInfo::SV_SF_3:
            firstPartTemp += QString::fromLatin1("/3");
            break;
        case QSysInfo::SV_SF_4:
            firstPartTemp += QString::fromLatin1("/4");
        default:
            break;
        }
#endif

        firstPartTemp += QString::fromLatin1("; ");

        // SSL support
#if !defined(QT_NO_OPENSSL)
        // we could check QSslSocket::supportsSsl() here, but this makes
        // OpenSSL, certificates etc being loaded in all cases were QWebPage
        // is used. This loading is not needed for non-https.
        firstPartTemp += QString::fromLatin1("U; ");
        // this may lead to a false positive: We indicate SSL since it is
        // compiled in even though supportsSsl() might return false
#else
        firstPartTemp += QString::fromLatin1("N; ");
#endif

        // Operating system
#ifdef Q_OS_AIX
        firstPartTemp += QString::fromLatin1("AIX");
#elif defined Q_OS_WIN32

        switch (QSysInfo::WindowsVersion) {
        case QSysInfo::WV_32s:
            firstPartTemp += QString::fromLatin1("Windows 3.1");
            break;
        case QSysInfo::WV_95:
            firstPartTemp += QString::fromLatin1("Windows 95");
            break;
        case QSysInfo::WV_98:
            firstPartTemp += QString::fromLatin1("Windows 98");
            break;
        case QSysInfo::WV_Me:
            firstPartTemp += QString::fromLatin1("Windows 98; Win 9x 4.90");
            break;
        case QSysInfo::WV_NT:
            firstPartTemp += QString::fromLatin1("WinNT4.0");
            break;
        case QSysInfo::WV_2000:
            firstPartTemp += QString::fromLatin1("Windows NT 5.0");
            break;
        case QSysInfo::WV_XP:
            firstPartTemp += QString::fromLatin1("Windows NT 5.1");
            break;
        case QSysInfo::WV_2003:
            firstPartTemp += QString::fromLatin1("Windows NT 5.2");
            break;
        case QSysInfo::WV_VISTA:
            firstPartTemp += QString::fromLatin1("Windows NT 6.0");
            break;
         case QSysInfo::WV_WINDOWS7:
            firstPartTemp += QString::fromLatin1("Windows NT 6.1");
            break;
         case QSysInfo::WV_CE:
            firstPartTemp += QString::fromLatin1("Windows CE");
            break;
         case QSysInfo::WV_CENET:
            firstPartTemp += QString::fromLatin1("Windows CE .NET");
            break;
         case QSysInfo::WV_CE_5:
            firstPartTemp += QString::fromLatin1("Windows CE 5.x");
            break;
         case QSysInfo::WV_CE_6:
            firstPartTemp += QString::fromLatin1("Windows CE 6.x");
            break;
        }

#elif defined Q_OS_DARWIN
#ifdef __i386__ || __x86_64__
        firstPartTemp += QString::fromLatin1("Intel Mac OS X");
#else
        firstPartTemp += QString::fromLatin1("PPC Mac OS X");
#endif

#elif defined Q_OS_BSDI
        firstPartTemp += QString::fromLatin1("BSD");
#elif defined Q_OS_BSD4
        firstPartTemp += QString::fromLatin1("BSD Four");
#elif defined Q_OS_CYGWIN
        firstPartTemp += QString::fromLatin1("Cygwin");
#elif defined Q_OS_DGUX
        firstPartTemp += QString::fromLatin1("DG/UX");
#elif defined Q_OS_DYNIX
        firstPartTemp += QString::fromLatin1("DYNIX/ptx");
#elif defined Q_OS_FREEBSD
        firstPartTemp += QString::fromLatin1("FreeBSD");
#elif defined Q_OS_HPUX
        firstPartTemp += QString::fromLatin1("HP-UX");
#elif defined Q_OS_HURD
        firstPartTemp += QString::fromLatin1("GNU Hurd");
#elif defined Q_OS_IRIX
        firstPartTemp += QString::fromLatin1("SGI Irix");
#elif defined Q_OS_LINUX

#if defined(__x86_64__)
        firstPartTemp += QString::fromLatin1("Linux x86_64");
#elif defined(__i386__)
        firstPartTemp += QString::fromLatin1("Linux i686");
#else
        firstPartTemp += QString::fromLatin1("Linux");
#endif

#elif defined Q_OS_LYNX
        firstPartTemp += QString::fromLatin1("LynxOS");
#elif defined Q_OS_NETBSD
        firstPartTemp += QString::fromLatin1("NetBSD");
#elif defined Q_OS_OS2
        firstPartTemp += QString::fromLatin1("OS/2");
#elif defined Q_OS_OPENBSD
        firstPartTemp += QString::fromLatin1("OpenBSD");
#elif defined Q_OS_OS2EMX
        firstPartTemp += QString::fromLatin1("OS/2");
#elif defined Q_OS_OSF
        firstPartTemp += QString::fromLatin1("HP Tru64 UNIX");
#elif defined Q_OS_QNX6
        firstPartTemp += QString::fromLatin1("QNX RTP Six");
#elif defined Q_OS_QNX
        firstPartTemp += QString::fromLatin1("QNX");
#elif defined Q_OS_RELIANT
        firstPartTemp += QString::fromLatin1("Reliant UNIX");
#elif defined Q_OS_SCO
        firstPartTemp += QString::fromLatin1("SCO OpenServer");
#elif defined Q_OS_SOLARIS
        firstPartTemp += QString::fromLatin1("Sun Solaris");
#elif defined Q_OS_ULTRIX
        firstPartTemp += QString::fromLatin1("DEC Ultrix");
#elif defined Q_WS_S60
        firstPartTemp += QLatin1Char(' ');
        QSysInfo::S60Version s60Version = QSysInfo::s60Version();
        switch (s60Version) {
        case QSysInfo::SV_S60_3_1:
            firstPartTemp += QString::fromLatin1("Series60/3.1");
            break;
        case QSysInfo::SV_S60_3_2:
            firstPartTemp += QString::fromLatin1("Series60/3.2");
            break;
        case QSysInfo::SV_S60_5_0:
            firstPartTemp += QString::fromLatin1("Series60/5.0");
            break;
        default:
            break;
        }
#elif defined Q_OS_UNIX
        firstPartTemp += QString::fromLatin1("UNIX BSD/SYSV system");
#elif defined Q_OS_UNIXWARE
        firstPartTemp += QString::fromLatin1("UnixWare Seven, Open UNIX Eight");
#else
        firstPartTemp += QString::fromLatin1("Unknown");
#endif

        // language is the split
        firstPartTemp += QString::fromLatin1("; ");
        firstPartTemp.squeeze();
        firstPart = firstPartTemp;

        QString secondPartTemp;
        secondPartTemp.reserve(150);
        secondPartTemp += QString::fromLatin1(") ");

        // webkit/qt version
        secondPartTemp += QString::fromLatin1("AppleWebKit/");
        secondPartTemp += qWebKitVersion();
        secondPartTemp += QString::fromLatin1(" (KHTML, like Gecko) ");


        // Application name split the third part
        secondPartTemp.squeeze();
        secondPart = secondPartTemp;

        QString thirdPartTemp;
        thirdPartTemp.reserve(150);
#if defined(Q_OS_SYMBIAN) || defined(Q_WS_MAEMO_5)
        thirdPartTemp += QLatin1String(" Mobile Safari/");
#else
        thirdPartTemp += QLatin1String(" Safari/");
#endif
        thirdPartTemp += qWebKitVersion();
        thirdPartTemp.squeeze();
        thirdPart = thirdPartTemp;
        Q_ASSERT(!firstPart.isNull());
        Q_ASSERT(!secondPart.isNull());
        Q_ASSERT(!thirdPart.isNull());
    }

    // Language
    QString languageName;
    if (d->client && d->client->ownerWidget())
        languageName = d->client->ownerWidget()->locale().name();
    else
        languageName = QLocale().name();
    languageName.replace(QLatin1Char('_'), QLatin1Char('-'));

    // Application name/version
    QString appName = QCoreApplication::applicationName();
    if (!appName.isEmpty()) {
        QString appVer = QCoreApplication::applicationVersion();
        if (!appVer.isEmpty())
            appName.append(QLatin1Char('/') + appVer);
    } else {
        // Qt version
        appName = QString::fromLatin1("Qt/") + QString::fromLatin1(qVersion());
    }

    return firstPart + languageName + secondPart + appName + thirdPart;
}


void QWebPagePrivate::_q_onLoadProgressChanged(int)
{
    m_totalBytes = page->progress()->totalPageAndResourceBytesToLoad();
    m_bytesReceived = page->progress()->totalBytesReceived();
}


/*!
    Returns the total number of bytes that were received from the network to render the current page,
    including extra content such as embedded images.

    \sa bytesReceived()
*/
quint64 QWebPage::totalBytes() const
{
    return d->m_totalBytes;
}


/*!
    Returns the number of bytes that were received from the network to render the current page.

    \sa totalBytes(), loadProgress()
*/
quint64 QWebPage::bytesReceived() const
{
    return d->m_bytesReceived;
}

/*!
    \since 4.7
    \fn void QWebPage::viewportChangeRequested(const QWebPage::ViewportHints& hints)

    This signal is emitted before any layout of the contents, giving you the viewport \a arguments
    the web page would like you to use when laying out its contents, including elements fixed to the
    viewport. This viewport might be larger that your actual viewport, meaning that a initialScaleFactor
    should be applied. When no scale is given, it is assumed that the contents should be scaled
    such that the width of the scaled contents fits within the actual viewport.

    The minimum and maximum allowed scale represents the min and max values that the page
    allows for scaling, and thus, affects the ability to zoom in on the page.

    Invalid values are supplied for the values not explicitly set by the web author; thus an
    invalid viewport size, and negative values for scale factor and limits. The boolean
    ViewportHints::isUserScalable is set to true.

    Page authors can provide the supplied values by using the viewport meta tag. More information
    about this can be found at \l{http://developer.apple.com/safari/library/documentation/appleapplications/reference/safariwebcontent/usingtheviewport/usingtheviewport.html}{Safari Reference Library: Using the Viewport Meta Tag}.

    \sa QWebPage::ViewportHints, setPreferredContentsSize(), QGraphicsWebView::setScale()
*/

/*!
    \fn void QWebPage::loadStarted()

    This signal is emitted when a new load of the page is started.

    \sa loadFinished()
*/

/*!
    \fn void QWebPage::loadProgress(int progress)

    This signal is emitted when the global progress status changes.
    The current value is provided by \a progress and scales from 0 to 100,
    which is the default range of QProgressBar.
    It accumulates changes from all the child frames.

    \sa bytesReceived()
*/

/*!
    \fn void QWebPage::loadFinished(bool ok)

    This signal is emitted when a load of the page is finished.
    \a ok will indicate whether the load was successful or any error occurred.

    \sa loadStarted(), ErrorPageExtension
*/

/*!
    \fn void QWebPage::linkHovered(const QString &link, const QString &title, const QString &textContent)

    This signal is emitted when the mouse hovers over a link.

    \a link contains the link url.
    \a title is the link element's title, if it is specified in the markup.
    \a textContent provides text within the link element, e.g., text inside an HTML anchor tag.

    When the mouse leaves the link element the signal is emitted with empty parameters.

    \sa linkClicked()
*/

/*!
    \fn void QWebPage::statusBarMessage(const QString& text)

    This signal is emitted when the statusbar \a text is changed by the page.
*/

/*!
    \fn void QWebPage::frameCreated(QWebFrame *frame)

    This signal is emitted whenever the page creates a new \a frame.

    \sa currentFrame()
*/

/*!
    \fn void QWebPage::selectionChanged()

    This signal is emitted whenever the selection changes, either interactively
    or programmatically (e.g. by calling triggerAction() with a selection action).

    \sa selectedText()
*/

/*!
    \fn void QWebPage::contentsChanged()
    \since 4.5

    This signal is emitted whenever the text in form elements changes
    as well as other editable content.

    \sa contentEditable, modified, QWebFrame::toHtml(), QWebFrame::toPlainText()
*/

/*!
    \fn void QWebPage::geometryChangeRequested(const QRect& geom)

    This signal is emitted whenever the document wants to change the position and size of the
    page to \a geom. This can happen for example through JavaScript.
*/

/*!
    \fn void QWebPage::repaintRequested(const QRect& dirtyRect)

    This signal is emitted whenever this QWebPage should be updated and no view was set.
    \a dirtyRect contains the area that needs to be updated. To paint the QWebPage get
    the mainFrame() and call the render(QPainter*, const QRegion&) method with the
    \a dirtyRect as the second parameter.

    \sa mainFrame()
    \sa view()
*/

/*!
    \fn void QWebPage::scrollRequested(int dx, int dy, const QRect& rectToScroll)

    This signal is emitted whenever the content given by \a rectToScroll needs
    to be scrolled \a dx and \a dy downwards and no view was set.

    \sa view()
*/

/*!
    \fn void QWebPage::windowCloseRequested()

    This signal is emitted whenever the page requests the web browser window to be closed,
    for example through the JavaScript \c{window.close()} call.
*/

/*!
    \fn void QWebPage::printRequested(QWebFrame *frame)

    This signal is emitted whenever the page requests the web browser to print \a frame,
    for example through the JavaScript \c{window.print()} call.

    \sa QWebFrame::print(), QPrintPreviewDialog
*/

/*!
    \fn void QWebPage::unsupportedContent(QNetworkReply *reply)

    This signal is emitted when WebKit cannot handle a link the user navigated to or a
    web server's response includes a "Content-Disposition" header with the 'attachment' 
    directive. If "Content-Disposition" is present in \a reply, the web server is indicating
    that the client should prompt the user to save the content regardless of content-type. 
    See RFC 2616 sections 19.5.1 for details about Content-Disposition.

    At signal emission time the meta-data of the QNetworkReply \a reply is available.

    \note This signal is only emitted if the forwardUnsupportedContent property is set to true.

    \sa downloadRequested()
*/

/*!
    \fn void QWebPage::downloadRequested(const QNetworkRequest &request)

    This signal is emitted when the user decides to download a link. The url of
    the link as well as additional meta-information is contained in \a request.

    \sa unsupportedContent()
*/

/*!
    \fn void QWebPage::microFocusChanged()

    This signal is emitted when for example the position of the cursor in an editable form
    element changes. It is used to inform input methods about the new on-screen position where
    the user is able to enter text. This signal is usually connected to the
    QWidget::updateMicroFocus() slot.
*/

/*!
    \fn void QWebPage::linkClicked(const QUrl &url)

    This signal is emitted whenever the user clicks on a link and the page's linkDelegationPolicy
    property is set to delegate the link handling for the specified \a url.

    By default no links are delegated and are handled by QWebPage instead.

    \note This signal possibly won't be emitted for clicked links which use
    JavaScript to trigger navigation.

    \sa linkHovered()
*/

/*!
    \fn void QWebPage::toolBarVisibilityChangeRequested(bool visible)

    This signal is emitted whenever the visibility of the toolbar in a web browser
    window that hosts QWebPage should be changed to \a visible.
*/

/*!
    \fn void QWebPage::statusBarVisibilityChangeRequested(bool visible)

    This signal is emitted whenever the visibility of the statusbar in a web browser
    window that hosts QWebPage should be changed to \a visible.
*/

/*!
    \fn void QWebPage::menuBarVisibilityChangeRequested(bool visible)

    This signal is emitted whenever the visibility of the menubar in a web browser
    window that hosts QWebPage should be changed to \a visible.
*/

/*!
    \fn void QWebPage::databaseQuotaExceeded(QWebFrame* frame, QString databaseName);
    \since 4.5

    This signal is emitted whenever the web site shown in \a frame is asking to store data
    to the database \a databaseName and the quota allocated to that web site is exceeded.

    \sa QWebDatabase
*/

/*!
  \since 4.5
  \fn void QWebPage::saveFrameStateRequested(QWebFrame* frame, QWebHistoryItem* item);

  This signal is emitted shortly before the history of navigated pages
  in \a frame is changed, for example when navigating back in the history.

  The provided QWebHistoryItem, \a item, holds the history entry of the frame before
  the change.

  A potential use-case for this signal is to store custom data in
  the QWebHistoryItem associated to the frame, using QWebHistoryItem::setUserData().
*/

/*!
  \since 4.5
  \fn void QWebPage::restoreFrameStateRequested(QWebFrame* frame);

  This signal is emitted when the load of \a frame is finished and the application may now update its state accordingly.
*/

/*!
  \fn QWebPagePrivate* QWebPage::handle() const
  \internal
*/

#include "moc_qwebpage.cpp"
