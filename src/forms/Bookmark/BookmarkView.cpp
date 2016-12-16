#include "BookmarkView.h"
#include "CubicSDR.h"

#include <wx/menu.h>
#include <wx/textdlg.h>
#include <algorithm>

#define wxCONTEXT_ADD_GROUP_ID 1000

#define BOOKMARK_VIEW_CHOICE_DEFAULT "Bookmark.."
#define BOOKMARK_VIEW_CHOICE_MOVE "Move to.."
#define BOOKMARK_VIEW_CHOICE_NEW "(New Group..)"

#define BOOKMARK_VIEW_STR_ADD_GROUP "Add Group"
#define BOOKMARK_VIEW_STR_ADD_GROUP_DESC "Enter Group Name"
#define BOOKMARK_VIEW_STR_UNNAMED "Unnamed"

BookmarkView::BookmarkView( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style) : BookmarkPanel(parent, id, pos, size, style) {

    rootBranch = m_treeView->AddRoot("Root");
    activeBranch = m_treeView->AppendItem(rootBranch, "Active");
    bookmarkBranch = m_treeView->AppendItem(rootBranch, "Bookmarks");
    recentBranch = m_treeView->AppendItem(rootBranch, "Recents");
    
    expandState["active"] = true;
    expandState["bookmark"] = true;
    expandState["recent"] = true;
    
    doUpdateActive.store(true);
    doUpdateBookmarks.store(true);
    bookmarkChoice = nullptr;
    dragItem = nullptr;
    dragItemId = nullptr;
    editingLabel = false;
    
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
    
    TreeViewItem *prevSel = itemToTVI(m_treeView->GetSelection());
    
    BookmarkNames groupNames;
    wxGetApp().getBookmarkMgr().getGroups(groupNames);

    if (doUpdateBookmarkGroup.size()) { // Nothing for the moment..
        doUpdateBookmarkGroup.erase(doUpdateBookmarkGroup.begin(), doUpdateBookmarkGroup.end());
    }

    wxTreeItemId bmSelFound = nullptr;
    
    std::map<std::string, bool> groupExpandState;
    
    for (auto g_i : groups) {
        groupExpandState[g_i.first] = m_treeView->IsExpanded(g_i.second);
    }
    
    groups.erase(groups.begin(),groups.end());
    m_treeView->DeleteChildren(bookmarkBranch);

    bool bmExpandState = expandState["bookmark"];

    for (auto gn_i : groupNames) {
        TreeViewItem* tvi = new TreeViewItem();
        tvi->type = TreeViewItem::TREEVIEW_ITEM_TYPE_GROUP;
        tvi->groupName = gn_i;
        wxTreeItemId group_itm = m_treeView->AppendItem(bookmarkBranch, gn_i);
        m_treeView->SetItemData(group_itm, tvi);
        groups[gn_i] = group_itm;
        if (prevSel != nullptr && prevSel->type == TreeViewItem::TREEVIEW_ITEM_TYPE_GROUP && gn_i == prevSel->groupName) {
            bmSelFound = group_itm;
        }
    }

    if (bmExpandState) {
        m_treeView->Expand(bookmarkBranch);
    }

    for (auto gn_i : groupNames) {
        wxTreeItemId groupItem = groups[gn_i];

        bool groupExpanded = false;
        
        if (groupExpandState.find(gn_i) != groupExpandState.end()) {
            groupExpanded = groupExpandState[gn_i];
        } else { // New
            groupExpanded = true;
        }

        BookmarkList bmList = wxGetApp().getBookmarkMgr().getBookmarks(gn_i);
        for (auto bmEnt : bmList) {
            TreeViewItem* tvi = new TreeViewItem();
            tvi->type = TreeViewItem::TREEVIEW_ITEM_TYPE_BOOKMARK;
            tvi->bookmarkEnt = bmEnt;
            tvi->groupName = gn_i;

            std::wstring labelVal = bmEnt->label;
            
            if (labelVal == "") {
                std::string freqStr = frequencyToStr(bmEnt->frequency) + " " + bmEnt->type;
                labelVal = wstring(freqStr.begin(),freqStr.end());
            }
            
            wxTreeItemId itm = m_treeView->AppendItem(groupItem, labelVal);
            m_treeView->SetItemData(itm, tvi);
            if (prevSel != nullptr && prevSel->type == TreeViewItem::TREEVIEW_ITEM_TYPE_BOOKMARK && prevSel->bookmarkEnt == bmEnt && groupExpanded) {
                bmSelFound = itm;
            }
        }

        if (groupExpanded) {
            m_treeView->Expand(groupItem);
        }
    }

    
    return bmSelFound;
}


void BookmarkView::doUpdateActiveList() {
    std::vector<DemodulatorInstance *> &demods = wxGetApp().getDemodMgr().getDemodulators();
    
    DemodulatorInstance *activeDemodulator = wxGetApp().getDemodMgr().getActiveDemodulator();
//    DemodulatorInstance *lastActiveDemodulator = wxGetApp().getDemodMgr().getLastActiveDemodulator();

    TreeViewItem *prevSel = itemToTVI(m_treeView->GetSelection());

    // Actives
    m_treeView->DeleteChildren(activeBranch);
    
    bool activeExpandState = expandState["active"];
    
    wxTreeItemId selItem = nullptr;
    for (auto demod_i : demods) {
        TreeViewItem* tvi = new TreeViewItem();
        tvi->type = TreeViewItem::TREEVIEW_ITEM_TYPE_ACTIVE;
        tvi->demod = demod_i;
        
        wxString activeLabel = demod_i->getDemodulatorUserLabel();
        if (activeLabel == "") {
            std::string wstr = frequencyToStr(demod_i->getFrequency()) + " " + demod_i->getDemodulatorType();
            activeLabel  = std::wstring(wstr.begin(),wstr.end());
        }
        
        wxTreeItemId itm = m_treeView->AppendItem(activeBranch,activeLabel);
        m_treeView->SetItemData(itm, tvi);
        
        if (activeDemodulator) {
            if (activeDemodulator == demod_i && activeExpandState) {
                selItem = itm;
            }
        }
        else if (prevSel != nullptr && prevSel->type == TreeViewItem::TREEVIEW_ITEM_TYPE_ACTIVE && prevSel->demod == demod_i && activeExpandState) {
            selItem = itm;
        }
    }

    bool recentExpandState = expandState["recent"];
    
    // Recents
    BookmarkList bmRecents = wxGetApp().getBookmarkMgr().getRecents();
    m_treeView->DeleteChildren(recentBranch);
    
    for (auto bmr_i: bmRecents) {
        TreeViewItem* tvi = new TreeViewItem();
        tvi->type = TreeViewItem::TREEVIEW_ITEM_TYPE_RECENT;
        tvi->bookmarkEnt = bmr_i;

        std::wstring labelVal;
        bmr_i->node->child("user_label")->element()->get(labelVal);
        if (labelVal == "") {
            std::string wstr = frequencyToStr(bmr_i->frequency) + " " + bmr_i->type;
            labelVal = std::wstring(wstr.begin(),wstr.end());
        }
        
        wxTreeItemId itm = m_treeView->AppendItem(recentBranch, labelVal);
        m_treeView->SetItemData(itm, tvi);
        if (prevSel && prevSel->type == TreeViewItem::TREEVIEW_ITEM_TYPE_RECENT && prevSel->bookmarkEnt == bmr_i && recentExpandState) {
            selItem = itm;
        }
    }
    
    if (activeExpandState) {
        m_treeView->Expand(activeBranch);
    }
    if (recentExpandState) {
        m_treeView->Expand(recentBranch);
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
    
    if (tvi->type == TreeViewItem::TREEVIEW_ITEM_TYPE_ACTIVE ||
        tvi->type == TreeViewItem::TREEVIEW_ITEM_TYPE_RECENT ||
        tvi->type == TreeViewItem::TREEVIEW_ITEM_TYPE_BOOKMARK ||
        tvi->type == TreeViewItem::TREEVIEW_ITEM_TYPE_GROUP)
    {
        event.Allow();
        editingLabel = true;
    } else {
        event.Veto();
    }
}


void BookmarkView::onTreeEndLabelEdit( wxTreeEvent& event ) {
    wxTreeItemId itm = event.GetItem();
    TreeViewItem* tvi = dynamic_cast<TreeViewItem*>(m_treeView->GetItemData(itm));
    
    std::wstring newText = m_treeView->GetEditControl()->GetValue().ToStdWstring();
    
    editingLabel = false;
    
    if (!tvi) {
        return;
    }
    
    if (tvi->type == TreeViewItem::TREEVIEW_ITEM_TYPE_ACTIVE) {
        tvi->demod->setDemodulatorUserLabel(newText);
        wxGetApp().getBookmarkMgr().updateActiveList();
    } else if (tvi->type == TreeViewItem::TREEVIEW_ITEM_TYPE_RECENT) {
        tvi->bookmarkEnt->label = newText;
        tvi->bookmarkEnt->node->child("user_label")->element()->set(newText);
        wxGetApp().getBookmarkMgr().updateActiveList();
    } else if (tvi->type == TreeViewItem::TREEVIEW_ITEM_TYPE_BOOKMARK) {
        tvi->bookmarkEnt->label = newText;
        tvi->bookmarkEnt->node->child("user_label")->element()->set(newText);
        wxGetApp().getBookmarkMgr().updateBookmarks();
    } else if (tvi->type == TreeViewItem::TREEVIEW_ITEM_TYPE_GROUP) {
        std::string newGroup = m_treeView->GetEditControl()->GetValue().ToStdString();
        wxGetApp().getBookmarkMgr().renameGroup(tvi->groupName, newGroup);
        wxGetApp().getBookmarkMgr().updateBookmarks();
    }
}


void BookmarkView::onTreeActivate( wxTreeEvent& event ) {
    wxTreeItemId itm = event.GetItem();
    TreeViewItem* tvi = dynamic_cast<TreeViewItem*>(m_treeView->GetItemData(itm));

    if (tvi) {
        if (tvi->type == TreeViewItem::TREEVIEW_ITEM_TYPE_ACTIVE) {
            if (!tvi->demod->isActive()) {
                wxGetApp().setFrequency(tvi->demod->getFrequency());
                wxGetApp().getDemodMgr().setActiveDemodulator(tvi->demod,false);
            }
        } else if (tvi->type == TreeViewItem::TREEVIEW_ITEM_TYPE_RECENT) {
            wxGetApp().getBookmarkMgr().removeRecent(tvi->bookmarkEnt);
            activateBookmark(tvi->bookmarkEnt);
            wxGetApp().getBookmarkMgr().updateActiveList();
        } else if (tvi->type == TreeViewItem::TREEVIEW_ITEM_TYPE_BOOKMARK) {
            activateBookmark(tvi->bookmarkEnt);
        }
    }
}


void BookmarkView::onTreeCollapse( wxTreeEvent& event ) {
    if (event.GetItem() == activeBranch) {
        expandState["active"] = false;
    } else if (event.GetItem() == bookmarkBranch) {
        expandState["bookmark"] = false;
    } else if (event.GetItem() == recentBranch) {
        expandState["recent"] = false;
    }

    event.Skip();
}


void BookmarkView::onTreeExpanded( wxTreeEvent& event ) {
    
    if (event.GetItem() == activeBranch) {
        expandState["active"] = true;
    } else if (event.GetItem() == bookmarkBranch) {
        expandState["bookmark"] = true;
    } else if (event.GetItem() == recentBranch) {
        expandState["recent"] = true;
    }

    event.Skip();
}


void BookmarkView::onTreeItemMenu( wxTreeEvent& event ) {
    if (m_treeView->GetSelection() == bookmarkBranch) {
        wxMenu menu;
        menu.Append(wxCONTEXT_ADD_GROUP_ID, BOOKMARK_VIEW_STR_ADD_GROUP);
        menu.Connect(wxCONTEXT_ADD_GROUP_ID, wxEVT_MENU, (wxObjectEventFunction)&BookmarkView::onMenuItem, nullptr, this);
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


void BookmarkView::doBookmarkActive(std::string group, DemodulatorInstance *demod) {
    wxGetApp().getBookmarkMgr().addBookmark(group, demod);
    wxGetApp().getBookmarkMgr().updateBookmarks();
    activeSelection(demod);
}


void BookmarkView::doBookmarkRecent(std::string group, BookmarkEntry *be) {
    wxGetApp().getBookmarkMgr().removeRecent(be);
    wxGetApp().getBookmarkMgr().addBookmark(group, be);
    wxGetApp().getBookmarkMgr().updateBookmarks();
    wxGetApp().getBookmarkMgr().updateActiveList();
    bookmarkSelection(be);
    
}


void BookmarkView::doMoveBookmark(BookmarkEntry *be, std::string group) {
    wxGetApp().getBookmarkMgr().moveBookmark(be, group);
    wxGetApp().getBookmarkMgr().updateBookmarks();
    bookmarkSelection(be);
}


void BookmarkView::updateBookmarkChoices() {
    if (!bookmarkChoices.empty()) {
        bookmarkChoices.erase(bookmarkChoices.begin(),bookmarkChoices.end());
    }
    TreeViewItem *activeSel = itemToTVI(m_treeView->GetSelection());
    
    bookmarkChoices.push_back(((activeSel != nullptr && activeSel->type == TreeViewItem::TREEVIEW_ITEM_TYPE_BOOKMARK))?BOOKMARK_VIEW_CHOICE_MOVE:BOOKMARK_VIEW_CHOICE_DEFAULT);
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

    TreeViewItem *tvi = itemToTVI(m_treeView->GetSelection());
    
    int numSel = bookmarkChoice->GetCount();
    int sel = bookmarkChoice->GetSelection();
    
    if (sel == 0) {
        return;
    }
    
    wxString stringVal = "";
    
    if (sel == (numSel-1)) {
        stringVal = wxGetTextFromUser(BOOKMARK_VIEW_STR_ADD_GROUP_DESC, BOOKMARK_VIEW_STR_ADD_GROUP, "");
    } else {
        stringVal = bookmarkChoices[sel];
    }
    
    if (stringVal == "") {
        return;
    }

    if (tvi != nullptr) {
        if (tvi->type == TreeViewItem::TREEVIEW_ITEM_TYPE_ACTIVE) {
            doBookmarkActive(stringVal.ToStdString(), tvi->demod);
        }
        if (tvi->type == TreeViewItem::TREEVIEW_ITEM_TYPE_RECENT) {
            doBookmarkRecent(stringVal.ToStdString(), tvi->bookmarkEnt);
        }
        if (tvi->type == TreeViewItem::TREEVIEW_ITEM_TYPE_BOOKMARK) {
            doMoveBookmark(tvi->bookmarkEnt, stringVal.ToStdString());
        }
    }
}


void BookmarkView::activeSelection(DemodulatorInstance *dsel) {
    
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
    
    TreeViewItem *sel = itemToTVI(m_treeView->GetSelection());

    long long freq = newDemod->getFrequency();
    long long currentFreq = wxGetApp().getFrequency();
    long long currentRate = wxGetApp().getSampleRate();
    
    if ( ( abs(freq - currentFreq) > currentRate / 2 ) || ( abs( currentFreq - freq) > currentRate / 2 ) ) {
        wxGetApp().setFrequency(freq);
    }
    
    newDemod->run();
    newDemod->setActive(true);
    wxGetApp().bindDemodulator(newDemod);
    doUpdateActiveList();
}


void BookmarkView::bookmarkSelection(BookmarkEntry *bmSel) {
    
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
    
    addBookmarkChoice(m_buttonPanel);
    addButton(m_buttonPanel, "Activate Bookmark", wxCommandEventHandler( BookmarkView::onActivateBookmark ));
    addButton(m_buttonPanel, "Remove Bookmark", wxCommandEventHandler( BookmarkView::onRemoveBookmark ));
   
    showProps();
    showButtons();
    refreshLayout();
}


void BookmarkView::recentSelection(BookmarkEntry *bmSel) {
    
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
    
    addBookmarkChoice(m_buttonPanel);
    addButton(m_buttonPanel, "Activate Recent", wxCommandEventHandler( BookmarkView::onActivateRecent ));
    
    showProps();
    showButtons();
    refreshLayout();
}

void BookmarkView::groupSelection(std::string groupName) {

    clearButtons();
    
    hideProps();

//    m_labelText->SetValue(groupSel);

//    m_labelText->Show();
//    m_labelLabel->Show();
    
    addButton(m_buttonPanel, "Remove Group", wxCommandEventHandler( BookmarkView::onRemoveGroup ));
    addButton(m_buttonPanel, "Rename Group", wxCommandEventHandler( BookmarkView::onRenameGroup ));
    
//    showProps();

    showButtons();
    refreshLayout();
}


void BookmarkView::bookmarkBranchSelection() {
    
    clearButtons();
    hideProps();
    
    addButton(m_buttonPanel, BOOKMARK_VIEW_STR_ADD_GROUP, wxCommandEventHandler( BookmarkView::onAddGroup ));
    
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
        if (tvi->demod->isActive()) {
            activeSelection(tvi->demod);
            wxGetApp().getDemodMgr().setActiveDemodulator(tvi->demod, false);
        }
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
    std::wstring newLabel = m_labelText->GetValue().ToStdWstring();
    TreeViewItem *curSel = itemToTVI(m_treeView->GetSelection());
    
    if (curSel != nullptr) {
        if (curSel->type == TreeViewItem::TREEVIEW_ITEM_TYPE_ACTIVE) {
            curSel->demod->setDemodulatorUserLabel(newLabel);
            wxGetApp().getBookmarkMgr().updateActiveList();
        } else if (curSel->type == TreeViewItem::TREEVIEW_ITEM_TYPE_BOOKMARK) {
            curSel->bookmarkEnt->label = m_labelText->GetValue().ToStdWstring();
            curSel->bookmarkEnt->node->child("user_label")->element()->set(newLabel);
            wxGetApp().getBookmarkMgr().updateBookmarks();
        } else if (curSel->type == TreeViewItem::TREEVIEW_ITEM_TYPE_RECENT) {
            curSel->bookmarkEnt->label = m_labelText->GetValue().ToStdWstring();
            curSel->bookmarkEnt->node->child("user_label")->element()->set(newLabel);
            wxGetApp().getBookmarkMgr().updateActiveList();
        }
    }

    //    else if (groupSel != "") {
//        std::string newGroupName = m_labelText->GetValue().ToStdString();
//        wxGetApp().getBookmarkMgr().renameGroup(groupSel, newGroupName);
//        groupSel = newGroupName;
//        wxGetApp().getBookmarkMgr().updateBookmarks();
//    }
}


void BookmarkView::onDoubleClickFreq( wxMouseEvent& event ) {
    TreeViewItem *curSel = itemToTVI(m_treeView->GetSelection());
    
    if (curSel && curSel->type == TreeViewItem::TREEVIEW_ITEM_TYPE_ACTIVE) {
        wxGetApp().getDemodMgr().setActiveDemodulator(curSel->demod, false);
        wxGetApp().showFrequencyInput(FrequencyDialog::FrequencyDialogTarget::FDIALOG_TARGET_DEFAULT);
    }
}


void BookmarkView::onDoubleClickBandwidth( wxMouseEvent& event ) {
    TreeViewItem *curSel = itemToTVI(m_treeView->GetSelection());

    if (curSel && curSel->type == TreeViewItem::TREEVIEW_ITEM_TYPE_ACTIVE) {
        wxGetApp().getDemodMgr().setActiveDemodulator(curSel->demod, false);
        wxGetApp().showFrequencyInput(FrequencyDialog::FrequencyDialogTarget::FDIALOG_TARGET_BANDWIDTH);
    }
}


void BookmarkView::onRemoveActive( wxCommandEvent& event ) {
    TreeViewItem *curSel = itemToTVI(m_treeView->GetSelection());

    if (curSel && curSel->type == TreeViewItem::TREEVIEW_ITEM_TYPE_ACTIVE) {
        if (editingLabel) {
            return;
        }
        wxGetApp().getDemodMgr().setActiveDemodulator(nullptr, false);
        wxGetApp().removeDemodulator(curSel->demod);
        wxGetApp().getDemodMgr().deleteThread(curSel->demod);
        m_treeView->Delete(m_treeView->GetSelection());
    }
}


void BookmarkView::onRemoveBookmark( wxCommandEvent& event ) {
    if (editingLabel) {
        return;
    }

    TreeViewItem *curSel = itemToTVI(m_treeView->GetSelection());

    if (curSel && curSel->type == TreeViewItem::TREEVIEW_ITEM_TYPE_BOOKMARK) {
        wxGetApp().getBookmarkMgr().removeBookmark(curSel->bookmarkEnt);
        m_treeView->Delete(m_treeView->GetSelection());
        wxGetApp().getBookmarkMgr().updateBookmarks();
    }
}


void BookmarkView::onActivateBookmark( wxCommandEvent& event ) {
    TreeViewItem *curSel = itemToTVI(m_treeView->GetSelection());

    if (curSel && curSel->type == TreeViewItem::TREEVIEW_ITEM_TYPE_BOOKMARK) {
        activateBookmark(curSel->bookmarkEnt);
    }
}


void BookmarkView::onActivateRecent( wxCommandEvent& event ) {
    TreeViewItem *curSel = itemToTVI(m_treeView->GetSelection());
    
    if (curSel && curSel->type == TreeViewItem::TREEVIEW_ITEM_TYPE_RECENT) {
        wxGetApp().getBookmarkMgr().removeRecent(curSel->bookmarkEnt);
        activateBookmark(curSel->bookmarkEnt);
        m_treeView->Delete(m_treeView->GetSelection());
        wxGetApp().getBookmarkMgr().updateActiveList();
    }
}


void BookmarkView::onAddGroup( wxCommandEvent& event ) {
    wxString stringVal = wxGetTextFromUser(BOOKMARK_VIEW_STR_ADD_GROUP_DESC, BOOKMARK_VIEW_STR_ADD_GROUP, "");
    if (stringVal.ToStdString() != "") {
        wxGetApp().getBookmarkMgr().addGroup(stringVal.ToStdString());
        wxGetApp().getBookmarkMgr().updateBookmarks();
    }
}


void BookmarkView::onRemoveGroup( wxCommandEvent& event ) {
    if (editingLabel) {
        return;
    }

    TreeViewItem *curSel = itemToTVI(m_treeView->GetSelection());

    if (curSel && curSel->type == TreeViewItem::TREEVIEW_ITEM_TYPE_GROUP) {
        wxGetApp().getBookmarkMgr().removeGroup(curSel->groupName);
        m_treeView->Delete(m_treeView->GetSelection());
        wxGetApp().getBookmarkMgr().updateBookmarks();
    }
}


void BookmarkView::onRenameGroup( wxCommandEvent& event ) {
    TreeViewItem *curSel = itemToTVI(m_treeView->GetSelection());
    
    if (!curSel || curSel->type != TreeViewItem::TREEVIEW_ITEM_TYPE_GROUP) {
        return;
    }
    
    wxString stringVal = "";
    stringVal = wxGetTextFromUser("Rename Group", "New Group Name", curSel->groupName);
    
    std::string newGroupName = stringVal.ToStdString();
    
    wxGetApp().getBookmarkMgr().renameGroup(curSel->groupName, newGroupName);

    m_treeView->Delete(m_treeView->GetSelection());

    wxGetApp().getBookmarkMgr().updateBookmarks();
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
                doBookmarkActive(BOOKMARK_VIEW_STR_UNNAMED, dragItem->demod);
            } else if (dragItem && dragItem->type == TreeViewItem::TREEVIEW_ITEM_TYPE_RECENT) {
                doBookmarkRecent(BOOKMARK_VIEW_STR_UNNAMED, dragItem->bookmarkEnt);
            } else if (dragItem && dragItem->type == TreeViewItem::TREEVIEW_ITEM_TYPE_BOOKMARK) {
                doMoveBookmark(dragItem->bookmarkEnt, BOOKMARK_VIEW_STR_UNNAMED);
            }
        }
        return;
    }
    
    if (tvi->type == TreeViewItem::TREEVIEW_ITEM_TYPE_GROUP) {
        if (dragItem && dragItem->type == TreeViewItem::TREEVIEW_ITEM_TYPE_ACTIVE) { // Active -> Group Item
            doBookmarkActive(tvi->groupName, dragItem->demod);
        } else if (dragItem && dragItem->type == TreeViewItem::TREEVIEW_ITEM_TYPE_RECENT) { // Recent -> Group Item
            doBookmarkRecent(tvi->groupName, dragItem->bookmarkEnt);
            m_treeView->Delete(dragItemId);
        } else if (dragItem && dragItem->type == TreeViewItem::TREEVIEW_ITEM_TYPE_BOOKMARK) { // Bookmark -> Group Item
            doMoveBookmark(dragItem->bookmarkEnt, tvi->groupName);
        }
    } else if (tvi->type == TreeViewItem::TREEVIEW_ITEM_TYPE_BOOKMARK) {
        if (dragItem && dragItem->type == TreeViewItem::TREEVIEW_ITEM_TYPE_ACTIVE) { // Active -> Same Group
            doBookmarkActive(tvi->groupName, dragItem->demod);
        } else if (dragItem && dragItem->type == TreeViewItem::TREEVIEW_ITEM_TYPE_RECENT) { // Recent -> Same Group
            doBookmarkRecent(tvi->groupName, dragItem->bookmarkEnt);
            m_treeView->Delete(dragItemId);
        } else if (dragItem && dragItem->type == TreeViewItem::TREEVIEW_ITEM_TYPE_BOOKMARK) { // Bookmark -> Same Group
            doMoveBookmark(dragItem->bookmarkEnt, tvi->groupName);
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

TreeViewItem *BookmarkView::itemToTVI(wxTreeItemId item) {
    TreeViewItem* tvi = nullptr;
    
    if (item != nullptr) {
        tvi = dynamic_cast<TreeViewItem*>(m_treeView->GetItemData(item));
    }
    
    return tvi;
}


