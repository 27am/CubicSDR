#include "SDRDevices.h"

#include <wx/textdlg.h>
#include <wx/msgdlg.h>

#include "CubicSDR.h"

SDRDevicesDialog::SDRDevicesDialog( wxWindow* parent ): devFrame( parent ) {
    refresh = true;
    m_addRemoteButton->Disable();
    m_useSelectedButton->Disable();
    m_deviceTimer.Start(250);
}

void SDRDevicesDialog::OnClose( wxCloseEvent& event ) {
    wxGetApp().setDeviceSelectorClosed();
    Destroy();
}

void SDRDevicesDialog::OnDeleteItem( wxTreeEvent& event ) {
    event.Skip();
}

void SDRDevicesDialog::OnSelectionChanged( wxTreeEvent& event ) {
    event.Skip();
}

void SDRDevicesDialog::OnAddRemote( wxMouseEvent& event ) {
    if (!SDREnumerator::hasRemoteModule()) {
        wxMessageDialog *info;
        info = new wxMessageDialog(NULL, wxT("Install SoapyRemote module to add remote servers.\n\nhttps://github.com/pothosware/SoapyRemote"), wxT("SoapyRemote not found."), wxOK | wxICON_ERROR);
        info->ShowModal();
        return;
    }
    
    wxString remoteAddr =
        wxGetTextFromUser("Remote Address (address[:port])\n\ni.e. 'raspberrypi.local', '192.168.1.103:1234'\n","SoapySDR Remote Address", "", this);

    if (!remoteAddr.Trim().empty()) {
        wxGetApp().addRemote(remoteAddr.Trim().ToStdString());
    }
    devTree->Disable();
    m_addRemoteButton->Disable();
    m_useSelectedButton->Disable();
    refresh = true;

}

void SDRDevicesDialog::OnUseSelected( wxMouseEvent& event ) {
    wxTreeItemId selId = devTree->GetSelection();
    
    devItems_i = devItems.find(selId);
    if (devItems_i != devItems.end()) {
        dev = devItems[selId];
        wxGetApp().setDevice(dev);
        Close();
    }
}

void SDRDevicesDialog::OnTreeDoubleClick( wxMouseEvent& event ) {
    OnUseSelected(event);
}

void SDRDevicesDialog::OnDeviceTimer( wxTimerEvent& event ) {
    if (refresh) {
        if (wxGetApp().areDevicesEnumerating() || !wxGetApp().areDevicesReady()) {
            std::string msg = wxGetApp().getNotification();
            devStatusBar->SetStatusText(msg);
            devTree->DeleteAllItems();
            devTree->AddRoot(msg);
            event.Skip();
            return;
        }
        
        devTree->DeleteAllItems();
        
        wxTreeItemId devRoot = devTree->AddRoot("Devices");
        wxTreeItemId localBranch = devTree->AppendItem(devRoot, "Local");
        wxTreeItemId remoteBranch = devTree->AppendItem(devRoot, "Remote");
        
        devs[""] = SDREnumerator::enumerate_devices("",true);
        if (devs[""] != NULL) {
            for (devs_i = devs[""]->begin(); devs_i != devs[""]->end(); devs_i++) {
                devItems[devTree->AppendItem(localBranch, (*devs_i)->getName())] = (*devs_i);
            }
        }
        
        std::vector<std::string> remotes = SDREnumerator::getRemotes();
        std::vector<std::string>::iterator remotes_i;
        std::vector<SDRDeviceInfo *>::iterator remoteDevs_i;
        
        if (remotes.size()) {
            for (remotes_i = remotes.begin(); remotes_i != remotes.end(); remotes_i++) {
                devs[*remotes_i] = SDREnumerator::enumerate_devices(*remotes_i, true);
                wxTreeItemId remoteNode = devTree->AppendItem(remoteBranch, *remotes_i);
                
                if (devs[*remotes_i] != NULL) {
                    for (remoteDevs_i = devs[*remotes_i]->begin(); remoteDevs_i != devs[*remotes_i]->end(); remoteDevs_i++) {
                        devItems[devTree->AppendItem(remoteNode, (*remoteDevs_i)->getName())] = (*remoteDevs_i);
                    }
                }
            }
        }
        
        m_addRemoteButton->Enable();
        m_useSelectedButton->Enable();
        devTree->Enable();
        devTree->ExpandAll();
        
        devStatusBar->SetStatusText("Ready.");

        refresh = false;
    }
}