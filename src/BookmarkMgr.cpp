#include "BookmarkMgr.h"
#include "CubicSDR.h"
#include "DataTree.h"


void BookmarkMgr::saveToFile(std::string bookmarkFn) {
    
    
}


void BookmarkMgr::loadFromFile(std::string bookmarkFn) {
    
    
}


void BookmarkMgr::addBookmark(std::string group, DemodulatorInstance *demod) {
    std::lock_guard < std::mutex > lock(busy_lock);
    
    BookmarkEntry *be = demodToBookmarkEntry(demod);
    
    wxGetApp().getDemodMgr().saveInstance(be->node, demod);
    
    bmData[group].push_back(be);
    bmDataSorted[group] = false;
}

void BookmarkMgr::addBookmark(std::string group, BookmarkEntry *be) {
    std::lock_guard < std::mutex > lock(busy_lock);
    
    bmData[group].push_back(be);
    bmDataSorted[group] = false;
}


void BookmarkMgr::removeBookmark(std::string group, BookmarkEntry *be) {
    std::lock_guard < std::mutex > lockData(busy_lock);
    std::lock_guard < std::mutex > lockEnt(be->busy_lock);

    if (bmData.find(group) == bmData.end()) {
        return;
    }

    BookmarkList::iterator i = std::find(bmData[group].begin(), bmData[group].end(), be);
    
    if (i != bmData[group].end()) {
        bmData[group].erase(i);
    }
}

void BookmarkMgr::removeBookmark(BookmarkEntry *be) {
    std::lock_guard < std::mutex > lockData(busy_lock);
    std::lock_guard < std::mutex > lockEnt(be->busy_lock);
    
    for (auto &bmd_i : bmData) {
        BookmarkList::iterator i = std::find(bmd_i.second.begin(), bmd_i.second.end(), be);
        if (i != bmd_i.second.end()) {
            bmd_i.second.erase(i);
        }
    }

}

BookmarkList BookmarkMgr::getBookmarks(std::string group) {
    std::lock_guard < std::mutex > lock(busy_lock);
    
    if (bmData.find(group) == bmData.end()) {
        BookmarkList results;
        return results;
    }
    
    if (!bmDataSorted[group]) {
        std::sort(bmData[group].begin(), bmData[group].end(), BookmarkEntryCompare());
        bmDataSorted[group] = true;
    }
    
    return bmData[group];
}

void BookmarkMgr::getGroups(BookmarkNames &arr) {
    for (BookmarkMap::iterator i = bmData.begin(); i!= bmData.end(); ++i) {
        arr.push_back(i->first);
    }
}

void BookmarkMgr::getGroups(wxArrayString &arr) {
    for (BookmarkMap::iterator i = bmData.begin(); i!= bmData.end(); ++i) {
        arr.push_back(i->first);
    }
}


void BookmarkMgr::updateActiveList() {
    BookmarkView *bmv = wxGetApp().getAppFrame()->getBookmarkView();
    
    if (bmv) {
        bmv->updateActiveList();
    }
}

void BookmarkMgr::updateBookmarks() {
    BookmarkView *bmv = wxGetApp().getAppFrame()->getBookmarkView();
    
    if (bmv) {
        bmv->updateBookmarks();
    }
}

void BookmarkMgr::updateBookmarks(std::string group) {
    BookmarkView *bmv = wxGetApp().getAppFrame()->getBookmarkView();
    
    if (bmv) {
        bmv->updateBookmarks(group);
    }
}


void BookmarkMgr::addRecent(DemodulatorInstance *demod) {
    std::lock_guard < std::mutex > lock(busy_lock);
    recents.push_back(demodToBookmarkEntry(demod));
    if (recents.size() > 10) {
        delete *(recents.begin());
        recents.erase(recents.begin(), recents.begin()+1);
    }
}


void BookmarkMgr::removeRecent(BookmarkEntry *be) {
    std::lock_guard < std::mutex > lock(busy_lock);
    
    BookmarkList::iterator bm_i = std::find(recents.begin(),recents.end(), be);
    
    if (bm_i != recents.end()) {
        recents.erase(bm_i);
    }
}


BookmarkList BookmarkMgr::getRecents() {
    return recents;
}


BookmarkEntry *BookmarkMgr::demodToBookmarkEntry(DemodulatorInstance *demod) {
    BookmarkEntry *be = new BookmarkEntry;
    
    be->bandwidth = demod->getBandwidth();
    be->type = demod->getDemodulatorType();
    be->label = demod->getDemodulatorUserLabel();
    if (be->label == "") {
        std::string wstr = demod->getLabel();
        be->label = std::wstring(wstr.begin(),wstr.end());
    }
    be->frequency = demod->getFrequency();

    be->node = new DataNode;
    wxGetApp().getDemodMgr().saveInstance(be->node, demod);
    
    return be;
}
