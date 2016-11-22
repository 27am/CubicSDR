#include "BookmarkView.h"
#include "CubicSDR.h"

#include <wx/menu.h>
#include <wx/textdlg.h>
#include <algorithm>

#define wxCONTEXT_ADD_GROUP_ID 1000

#define BOOKMARK_VIEW_CHOICE_DEFAULT "Bookmark.."
#define BOOKMARK_VIEW_CHOICE_NEW "(New Group..)"

BookmarkView::BookmarkView( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style) : BookmarkPanel(parent, id, pos, size, style) {

    rootBranch = m_treeView->AddRoot("Root");
    activeBranch = m_treeView->AppendItem(rootBranch, "Active");
    bookmarkBranch = m_treeView->AppendItem(rootBranch, "Bookmarks");
    recentBranch = m_treeView->AppendItem(rootBranch, "Recents");
    
    doUpdateActive.store(true);
    doUpdateBookmarks.store(true);
    bookmarkChoice = nullptr;
    activeSel = nullptr;
    recentSel = nullptr;
    dragItem = nullptr;
    dragItemId = nullptr;
    
    hideProps();
    
    m_updateTimer.Start(500);
    mouseInView.store(false);

}


void BookmarkView::onUpdateTimer( wxTimerEvent& event ) {
    if (doUpdateActive.load()) {
        doUpdateActiveList();
        
        doUpdateActive.store(false);
    }
    if (doUpdateBookmarks.load()) {
        wxTreeItemId bmSel = refreshBookmarks();
        if (bmSel) {
            m_treeView->SelectItem(bmSel);
        }
        doUpdateBookmarks.store(false);
    }
}


void BookmarkView::updateTheme() {
    wxColour bgColor(ThemeMgr::mgr.currentTheme->generalBackground);
    wxColour textColor(ThemeMgr::mgr.currentTheme->text);
    wxColour btn(ThemeMgr::mgr.currentTheme->button);
    wxColour btnHl(ThemeMgr::mgr.currentTheme->buttonHighlight);
    
    SetBackgroundColour(bgColor);

    m_treeView->SetBackgroundColour(bgColor);
    m_treeView->SetForegroundColour(textColor);
    
    m_propPanel->SetBackgroundColour(bgColor);
    m_propPanel->SetForegroundColour(textColor);
    
    m_labelLabel->SetForegroundColour(textColor);
    m_frequencyVal->SetForegroundColour(textColor);
    m_frequencyLabel->SetForegroundColour(textColor);
    m_bandwidthVal->SetForegroundColour(textColor);
    m_bandwidthLabel->SetForegroundColour(textColor);
    m_modulationVal->SetForegroundColour(textColor);
    m_modulationLabel->SetForegroundColour(textColor);

    m_buttonPanel->SetBackgroundColour(bgColor);
}


void BookmarkView::updateActiveList() {
    doUpdateActive.store(true);
}


void BookmarkView::updateBookmarks() {
    doUpdateBookmarks.store(true);
}


void BookmarkView::updateBookmarks(std::string group) {
    doUpdateBookmarkGroup.insert(group);
    doUpdateBookmarks.store(true);
}


wxTreeItemId BookmarkView::refreshBookmarks() {
    
    BookmarkNames groupNames;
    wxGetApp().getBookmarkMgr().getGroups(groupNames);

    if (doUpdateBookmarkGroup.size()) { // Nothing for the moment..
        doUpdateBookmarkGroup.erase(doUpdateBookmarkGroup.begin(), doUpdateBookmarkGroup.end());
    }

    wxTreeItemId bmSelFound = nullptr;
    for (auto gn_i : groupNames) {
        if (groups.find(gn_i) == groups.end()) {
            TreeViewItem* tvi = new TreeViewItem();
            tvi->type = TreeViewItem::TREEVIEW_ITEM_TYPE_GROUP;
            tvi->groupName = gn_i;
            wxTreeItemId itm = m_treeView->AppendItem(bookmarkBranch, gn_i);
            m_treeView->SetItemData(itm, tvi);
            groups[gn_i] = itm;
        }
        
        wxTreeItemId groupItem = groups[gn_i];
        m_treeView->DeleteChildren(groupItem);
        
        BookmarkList bmList = wxGetApp().getBookmarkMgr().getBookmarks(gn_i);
        for (auto bmEnt : bmList) {
            TreeViewItem* tvi = new TreeViewItem();
            tvi->type = TreeViewItem::TREEVIEW_ITEM_TYPE_BOOKMARK;
            tvi->bookmarkEnt = bmEnt;
            tvi->groupName = gn_i;
            wxTreeItemId itm = m_treeView->AppendItem(groupItem, bmEnt->label);
            m_treeView->SetItemData(itm, tvi);
            if (bookmarkSel == bmEnt) {
                bmSelFound = itm;
            }
        }
    }
    
    return bmSelFound;
}


void BookmarkView::doUpdateActiveList() {
    std::vector<DemodulatorInstance *> &demods = wxGetApp().getDemodMgr().getDemodulators();
    
    DemodulatorInstance *activeDemodulator = wxGetApp().getDemodMgr().getActiveDemodulator();
//    DemodulatorInstance *lastActiveDemodulator = wxGetApp().getDemodMgr().getLastActiveDemodulator();

    // Actives
    m_treeView->DeleteChildren(activeBranch);
    
    wxTreeItemId selItem = nullptr;
    for (auto demod_i : demods) {
        TreeViewItem* tvi = new TreeViewItem();
        tvi->type = TreeViewItem::TREEVIEW_ITEM_TYPE_ACTIVE;
        tvi->demod = demod_i;
        
        wxString activeLabel = demod_i->getDemodulatorUserLabel();
        if (activeLabel == "") {
            activeLabel = demod_i->getLabel();
        }
        
        wxTreeItemId itm = m_treeView->AppendItem(activeBranch,activeLabel);
        m_treeView->SetItemData(itm, tvi);
        
        if (activeDemodulator) {
            if (activeDemodulator == demod_i) {
                selItem = itm;
                activeSel = demod_i;
            }
        }
        else if (activeSel == demod_i) {
            selItem = itm;
        }
    }
    
    // Recents
    BookmarkList bmRecents = wxGetApp().getBookmarkMgr().getRecents();
    m_treeView->DeleteChildren(recentBranch);
    
    for (auto bmr_i: bmRecents) {
        TreeViewItem* tvi = new TreeViewItem();
        tvi->type = TreeViewItem::TREEVIEW_ITEM_TYPE_RECENT;
        tvi->bookmarkEnt = bmr_i;

        wxTreeItemId itm = m_treeView->AppendItem(recentBranch, bmr_i->label);
        m_treeView->SetItemData(itm, tvi);
        if (recentSel == bmr_i) {
            selItem = itm;
        }
    }

    if (selItem != nullptr) {
        m_treeView->SelectItem(selItem);
    }
}


void BookmarkView::onTreeBeginLabelEdit( wxTreeEvent& event ) {
    TreeViewItem* tvi = dynamic_cast<TreeViewItem*>(m_treeView->GetItemData(event.GetItem()));

    if (!tvi) {
        event.Veto();
        return;
    }
    
    if (tvi->type == TreeViewItem::TREEVIEW_ITEM_TYPE_ACTIVE) {
        event.Allow();
    } else if (tvi->type == TreeViewItem::TREEVIEW_ITEM_TYPE_RECENT) {
        event.Veto();
    } else if (tvi->type == TreeViewItem::TREEVIEW_ITEM_TYPE_BOOKMARK) {
        event.Allow();
    } else if (tvi->type == TreeViewItem::TREEVIEW_ITEM_TYPE_GROUP) {
        event.Allow();
    } else {
        event.Veto();
    }
}


void BookmarkView::onTreeEndLabelEdit( wxTreeEvent& event ) {
    event.Skip();
}


void BookmarkView::onTreeActivate( wxTreeEvent& event ) {
    if (recentSel) {
        activateBookmark(recentSel);
    }
    if (bookmarkSel) {
        activateBookmark(bookmarkSel);
    }
}


void BookmarkView::onTreeCollapse( wxTreeEvent& event ) {
    event.Skip();
}


void BookmarkView::onTreeExpanded( wxTreeEvent& event ) {
    event.Skip();
}


void BookmarkView::onTreeItemMenu( wxTreeEvent& event ) {
    if (m_treeView->GetSelection() == bookmarkBranch) {
        wxMenu menu;
        menu.Append(wxCONTEXT_ADD_GROUP_ID, "Add Group");
        menu.Connect(wxCONTEXT_ADD_GROUP_ID, wxEVT_MENU, (wxObjectEventFunction)&BookmarkView::onMenuItem);
        PopupMenu(&menu);
    }
}


void BookmarkView::onMenuItem(wxCommandEvent& event) {
    if (event.GetId() == wxCONTEXT_ADD_GROUP_ID) {
        onAddGroup(event);
    }
}


bool BookmarkView::isMouseInView() {
    return mouseInView.load();
}


void BookmarkView::hideProps() {
    m_frequencyLabel->Hide();
    m_frequencyVal->Hide();
    
    m_bandwidthLabel->Hide();
    m_bandwidthVal->Hide();
    
    m_modulationVal->Hide();
    m_modulationLabel->Hide();
    
    m_labelText->Hide();
    m_labelLabel->Hide();
    
    m_propPanel->Hide();
    m_buttonPanel->Hide();
}


void BookmarkView::showProps() {
    m_propPanel->Show();
    m_propPanel->GetSizer()->Layout();
}


void BookmarkView::clearButtons() {
    m_buttonPanel->Hide();
    m_buttonPanel->DestroyChildren();
    bookmarkChoice = nullptr;
}

void BookmarkView::showButtons() {
    m_buttonPanel->Show();
    m_buttonPanel->GetSizer()->Layout();
}

void BookmarkView::refreshLayout() {
    GetSizer()->Layout();
    Update();
}


wxButton *BookmarkView::makeButton(wxWindow *parent, std::string labelVal, wxObjectEventFunction handler) {
    wxButton *nButton = new wxButton( m_buttonPanel, wxID_ANY, labelVal);
    nButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, handler, nullptr, this);
    nButton->SetBackgroundColour(ThemeMgr::mgr.currentTheme->generalBackground);
    return nButton;
}


wxButton *BookmarkView::addButton(wxWindow *parent, std::string labelVal, wxObjectEventFunction handler) {
    wxButton *nButton = makeButton(parent, labelVal, handler);
    parent->GetSizer()->Add( nButton, 0, wxEXPAND);
    return nButton;
}

void BookmarkView::updateBookmarkChoices() {
    if (!bookmarkChoices.empty()) {
        bookmarkChoices.erase(bookmarkChoices.begin(),bookmarkChoices.end());
    }
    bookmarkChoices.push_back(BOOKMARK_VIEW_CHOICE_DEFAULT);
    wxGetApp().getBookmarkMgr().getGroups(bookmarkChoices);
    bookmarkChoices.push_back(BOOKMARK_VIEW_CHOICE_NEW);
}

void BookmarkView::addBookmarkChoice(wxWindow *parent) {
    updateBookmarkChoices();
    bookmarkChoice = new wxChoice(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, bookmarkChoices, wxALL|wxEXPAND, wxDefaultValidator, "Bookmark");
    bookmarkChoice->Connect( wxEVT_COMMAND_CHOICE_SELECTED, (wxObjectEventFunction)&BookmarkView::onBookmarkChoice, nullptr, this);
    parent->GetSizer()->Add(bookmarkChoice, 0, wxALL | wxEXPAND);
}


void BookmarkView::onBookmarkChoice( wxCommandEvent &event ) {
    
    int numSel = bookmarkChoice->GetCount();
    int sel = bookmarkChoice->GetSelection();
    
    if (sel == 0) {
        return;
    }
    
    wxString stringVal = "";
    
    if (sel == (numSel-1)) {
        stringVal = wxGetTextFromUser("Enter Group Name", "Add Group", "");
    } else {
        stringVal = bookmarkChoices[sel];
    }
    
    if (stringVal == "") {
        return;
    }

    groupSel = stringVal;

    if (activeSel) {
        wxGetApp().getBookmarkMgr().addBookmark(groupSel, activeSel);
        wxGetApp().getBookmarkMgr().updateBookmarks();
        activeSelection(activeSel);
    }
    if (recentSel) {
        wxGetApp().getBookmarkMgr().removeRecent(recentSel);
        wxGetApp().getBookmarkMgr().addBookmark(groupSel, recentSel);
        wxGetApp().getBookmarkMgr().updateBookmarks();
        wxGetApp().getBookmarkMgr().updateActiveList();
        bookmarkSelection(recentSel);
    }
}


void BookmarkView::activeSelection(DemodulatorInstance *dsel) {
    activeSel = dsel;
    recentSel = nullptr;
    
    m_frequencyVal->SetLabelText(frequencyToStr(dsel->getFrequency()));
    m_bandwidthVal->SetLabelText(frequencyToStr(dsel->getBandwidth()));
    m_modulationVal->SetLabelText(dsel->getDemodulatorType());
    m_labelText->SetValue(dsel->getDemodulatorUserLabel());
    
    hideProps();
    
    m_frequencyVal->Show();
    m_frequencyLabel->Show();
    
    m_bandwidthVal->Show();
    m_bandwidthLabel->Show();
    
    m_modulationVal->Show();
    m_modulationLabel->Show();
    
    m_labelText->Show();
    m_labelLabel->Show();
    
    clearButtons();

    addBookmarkChoice(m_buttonPanel);
    addButton(m_buttonPanel, "Remove Active", wxCommandEventHandler( BookmarkView::onRemoveActive ));

    showProps();
    showButtons();
    refreshLayout();
}


void BookmarkView::activateBookmark(BookmarkEntry *bmEnt) {
    DemodulatorInstance *newDemod = wxGetApp().getDemodMgr().loadInstance(bmEnt->node);
    newDemod->run();
    newDemod->setActive(true);
    wxGetApp().bindDemodulator(newDemod);
    if (bmEnt == recentSel) {
        activeSel = newDemod;
        recentSel = nullptr;
    }
    doUpdateActiveList();
}


void BookmarkView::bookmarkSelection(BookmarkEntry *bmSel) {
    bookmarkSel = bmSel;
    recentSel = nullptr;
    activeSel = nullptr;
    
    m_frequencyVal->SetLabelText(frequencyToStr(bmSel->frequency));
    m_bandwidthVal->SetLabelText(frequencyToStr(bmSel->bandwidth));
    m_modulationVal->SetLabelText(bmSel->type);
    m_labelText->SetValue(bmSel->label);
    
    hideProps();
    
    m_frequencyVal->Show();
    m_frequencyLabel->Show();
    
    m_bandwidthVal->Show();
    m_bandwidthLabel->Show();
    
    m_modulationVal->Show();
    m_modulationLabel->Show();
    
    m_labelText->Show();
    m_labelLabel->Show();
    
    clearButtons();
    
    addButton(m_buttonPanel, "Activate Bookmark", wxCommandEventHandler( BookmarkView::onActivateBookmark ));
    addButton(m_buttonPanel, "Remove Bookmark", wxCommandEventHandler( BookmarkView::onRemoveBookmark ));
    
    showProps();
    showButtons();
    refreshLayout();
}


void BookmarkView::recentSelection(BookmarkEntry *bmSel) {
    recentSel = bmSel;
    activeSel = nullptr;
    bookmarkSel = nullptr;
    
    m_frequencyVal->SetLabelText(frequencyToStr(bmSel->frequency));
    m_bandwidthVal->SetLabelText(frequencyToStr(bmSel->bandwidth));
    m_modulationVal->SetLabelText(bmSel->type);
    m_labelText->SetValue(bmSel->label);
    
    hideProps();
    
    m_frequencyVal->Show();
    m_frequencyLabel->Show();
    
    m_bandwidthVal->Show();
    m_bandwidthLabel->Show();
    
    m_modulationVal->Show();
    m_modulationLabel->Show();
    
    m_labelText->Show();
    m_labelLabel->Show();
    
    clearButtons();
    
    addButton(m_buttonPanel, "Activate Recent", wxCommandEventHandler( BookmarkView::onActivateRecent ));
    addBookmarkChoice(m_buttonPanel);
    
    showProps();
    showButtons();
    refreshLayout();
}

void BookmarkView::groupSelection(std::string groupName) {
    recentSel = nullptr;
    activeSel = nullptr;
    bookmarkSel = nullptr;
    groupSel = groupName;

    clearButtons();
    
    hideProps();

    addButton(m_buttonPanel, "Remove Group", wxCommandEventHandler( BookmarkView::onRemoveGroup ));
    addButton(m_buttonPanel, "Rename Group", wxCommandEventHandler( BookmarkView::onRenameGroup ));
    
    showButtons();
    refreshLayout();
}


void BookmarkView::bookmarkBranchSelection() {
    recentSel = nullptr;
    activeSel = nullptr;
    bookmarkSel = nullptr;
    
    clearButtons();
    hideProps();
    
    addButton(m_buttonPanel, "Add Group", wxCommandEventHandler( BookmarkView::onAddGroup ));
    
    showButtons();
    refreshLayout();
}


void BookmarkView::recentBranchSelection() {
    hideProps();
    this->Layout();
}


void BookmarkView::activeBranchSelection() {
    hideProps();
    this->Layout();
}


void BookmarkView::onTreeSelect( wxTreeEvent& event ) {
    wxTreeItemId itm = event.GetItem();
    TreeViewItem* tvi = dynamic_cast<TreeViewItem*>(m_treeView->GetItemData(itm));

    if (!tvi) {
        
        if (itm == bookmarkBranch) {
            bookmarkBranchSelection();
        } else if (itm == activeBranch) {
            activeBranchSelection();
        } else if (itm == recentBranch) {
            recentBranchSelection();
        } else {
            hideProps();
            this->Layout();
        }
        
        return;
    }
                                                    
    if (tvi->type == TreeViewItem::TREEVIEW_ITEM_TYPE_ACTIVE) {
        activeSelection(tvi->demod);
        wxGetApp().getDemodMgr().setActiveDemodulator(tvi->demod, false);
    } else if (tvi->type == TreeViewItem::TREEVIEW_ITEM_TYPE_RECENT) {
        recentSelection(tvi->bookmarkEnt);
    } else if (tvi->type == TreeViewItem::TREEVIEW_ITEM_TYPE_BOOKMARK) {
        bookmarkSelection(tvi->bookmarkEnt);
    } else if (tvi->type == TreeViewItem::TREEVIEW_ITEM_TYPE_GROUP) {
        groupSelection(tvi->groupName);
    } else {
        hideProps();
        this->Layout();
    }
}


void BookmarkView::onTreeSelectChanging( wxTreeEvent& event ) {
    event.Skip();
}


void BookmarkView::onLabelText( wxCommandEvent& event ) {
    event.Skip();
}


void BookmarkView::onDoubleClickFreq( wxMouseEvent& event ) {
    if (activeSel) {
        wxGetApp().getDemodMgr().setActiveDemodulator(activeSel, false);
        wxGetApp().showFrequencyInput(FrequencyDialog::FrequencyDialogTarget::FDIALOG_TARGET_DEFAULT);
    }
}


void BookmarkView::onDoubleClickBandwidth( wxMouseEvent& event ) {
    if (activeSel) {
        wxGetApp().getDemodMgr().setActiveDemodulator(activeSel, false);
        wxGetApp().showFrequencyInput(FrequencyDialog::FrequencyDialogTarget::FDIALOG_TARGET_BANDWIDTH);
    }
}


void BookmarkView::onRemoveActive( wxCommandEvent& event ) {
    if (activeSel != nullptr) {
        wxGetApp().getDemodMgr().setActiveDemodulator(nullptr, false);
        wxGetApp().removeDemodulator(activeSel);
        wxGetApp().getDemodMgr().deleteThread(activeSel);
        activeSel = nullptr;
    }
}


void BookmarkView::onRemoveBookmark( wxCommandEvent& event ) {
    if (bookmarkSel) {
        wxGetApp().getBookmarkMgr().removeBookmark(bookmarkSel);
        bookmarkSel = nullptr;
        wxGetApp().getBookmarkMgr().updateBookmarks();
    }
}


void BookmarkView::onActivateBookmark( wxCommandEvent& event ) {
    if (bookmarkSel) {
        activateBookmark(bookmarkSel);
    }
}


void BookmarkView::onActivateRecent( wxCommandEvent& event ) {
    if (recentSel) {
        activateBookmark(recentSel);
    }
}


void BookmarkView::onAddGroup( wxCommandEvent& event ) {
    wxString stringVal = wxGetTextFromUser("Enter Group Name", "Add Group", "");
    if (stringVal.ToStdString() != "") {
        wxGetApp().getBookmarkMgr().getBookmarks(stringVal.ToStdString());
        wxGetApp().getBookmarkMgr().updateBookmarks();
        groupSel = stringVal;
    }
}

void BookmarkView::onRemoveGroup( wxCommandEvent& event ) {
    
}


void BookmarkView::onRenameGroup( wxCommandEvent& event ) {
    
}


void BookmarkView::onTreeBeginDrag( wxTreeEvent& event ) {
    TreeViewItem* tvi = dynamic_cast<TreeViewItem*>(m_treeView->GetItemData(event.GetItem()));
    
    dragItem = nullptr;
    dragItemId = nullptr;
    
    if (!tvi) {
        event.Veto();
        return;
    }
    
    bool bAllow = false;
    std::wstring dragItemName;
    
    if (tvi->type == TreeViewItem::TREEVIEW_ITEM_TYPE_ACTIVE) {
        bAllow = true;
        dragItemName = tvi->demod->getDemodulatorUserLabel();
        if (dragItemName == "") {
            std::string wstr = tvi->demod->getLabel();
            dragItemName = std::wstring(wstr.begin(),wstr.end());
        }
    } else if (tvi->type == TreeViewItem::TREEVIEW_ITEM_TYPE_RECENT) {
        bAllow = true;
        dragItemName = tvi->bookmarkEnt->label;
    } else if (tvi->type == TreeViewItem::TREEVIEW_ITEM_TYPE_BOOKMARK) {
        bAllow = true;
        dragItemName = tvi->bookmarkEnt->label;
    }
    
    if (bAllow) {
        wxColour bgColor(ThemeMgr::mgr.currentTheme->generalBackground);
        wxColour textColor(ThemeMgr::mgr.currentTheme->text);
        
        m_treeView->SetBackgroundColour(textColor);
        m_treeView->SetForegroundColour(bgColor);
        m_treeView->SetToolTip("Dragging " + dragItemName);
        
        dragItem = tvi;
        dragItemId = event.GetItem();

        event.Allow();
    } else {
        event.Veto();
    }
}


void BookmarkView::onTreeEndDrag( wxTreeEvent& event ) {

    wxColour bgColor(ThemeMgr::mgr.currentTheme->generalBackground);
    wxColour textColor(ThemeMgr::mgr.currentTheme->text);
    
    m_treeView->SetBackgroundColour(bgColor);
    m_treeView->SetForegroundColour(textColor);
    m_treeView->UnsetToolTip();

    if (!event.GetItem()) {
        event.Veto();
        return;
    }

    TreeViewItem* tvi = dynamic_cast<TreeViewItem*>(m_treeView->GetItemData(event.GetItem()));

    if (!tvi) {
        if (event.GetItem() == bookmarkBranch) {
            if (dragItem && dragItem->type == TreeViewItem::TREEVIEW_ITEM_TYPE_ACTIVE) {
                wxGetApp().getBookmarkMgr().addBookmark("Ungrouped", dragItem->demod);
                wxGetApp().getBookmarkMgr().updateBookmarks();
            } else if (dragItem && dragItem->type == TreeViewItem::TREEVIEW_ITEM_TYPE_RECENT) {
                wxGetApp().getBookmarkMgr().removeRecent(dragItem->bookmarkEnt);
                wxGetApp().getBookmarkMgr().addBookmark("Ungrouped", dragItem->bookmarkEnt);
                m_treeView->Delete(dragItemId);
                wxGetApp().getBookmarkMgr().updateBookmarks();
                wxGetApp().getBookmarkMgr().updateActiveList();
            }
        }
        
        return;
    }
    
    if (tvi->type == TreeViewItem::TREEVIEW_ITEM_TYPE_GROUP) {
        if (dragItem && dragItem->type == TreeViewItem::TREEVIEW_ITEM_TYPE_ACTIVE) { // Active -> Group Item
            wxGetApp().getBookmarkMgr().addBookmark(tvi->groupName, dragItem->demod);
            wxGetApp().getBookmarkMgr().updateBookmarks();
        } else if (dragItem && dragItem->type == TreeViewItem::TREEVIEW_ITEM_TYPE_RECENT) { // Recent -> Group Item
            wxGetApp().getBookmarkMgr().removeRecent(dragItem->bookmarkEnt);
            wxGetApp().getBookmarkMgr().addBookmark(tvi->groupName, dragItem->bookmarkEnt);
            m_treeView->Delete(dragItemId);
            wxGetApp().getBookmarkMgr().updateBookmarks();
            wxGetApp().getBookmarkMgr().updateActiveList();
        }
    } else if (tvi->type == TreeViewItem::TREEVIEW_ITEM_TYPE_BOOKMARK) {
        if (dragItem && dragItem->type == TreeViewItem::TREEVIEW_ITEM_TYPE_ACTIVE) { // Active -> Same Group
            wxGetApp().getBookmarkMgr().addBookmark(tvi->groupName, dragItem->demod);
            wxGetApp().getBookmarkMgr().updateBookmarks();
        } else if (dragItem && dragItem->type == TreeViewItem::TREEVIEW_ITEM_TYPE_RECENT) { // Recent -> Same Group
            wxGetApp().getBookmarkMgr().removeRecent(dragItem->bookmarkEnt);
            wxGetApp().getBookmarkMgr().addBookmark(tvi->groupName, dragItem->bookmarkEnt);
            m_treeView->Delete(dragItemId);
            wxGetApp().getBookmarkMgr().updateBookmarks();
            wxGetApp().getBookmarkMgr().updateActiveList();
        }
    }
}


void BookmarkView::onTreeDeleteItem( wxTreeEvent& event ) {
    event.Skip();
}


void BookmarkView::onTreeItemGetTooltip( wxTreeEvent& event ) {
    
    event.Skip();
}


void BookmarkView::onEnterWindow( wxMouseEvent& event ) {
    mouseInView.store(true);
}


void BookmarkView::onLeaveWindow( wxMouseEvent& event ) {
    mouseInView.store(false);
}
