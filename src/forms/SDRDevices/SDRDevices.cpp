#include "SDRDevices.h"

#include <wx/textdlg.h>
#include <wx/msgdlg.h>

#include "CubicSDR.h"

SDRDevicesDialog::SDRDevicesDialog( wxWindow* parent ): devFrame( parent ) {
    refresh = true;
    failed = false;
    m_refreshButton->Disable();
    m_addRemoteButton->Disable();
    m_useSelectedButton->Disable();
    m_deviceTimer.Start(250);
    selId = 0;
}

void SDRDevicesDialog::OnClose( wxCloseEvent& event ) {
    wxGetApp().setDeviceSelectorClosed();
    Destroy();
}

void SDRDevicesDialog::OnDeleteItem( wxTreeEvent& event ) {
    event.Skip();
}

wxPGProperty *SDRDevicesDialog::addArgInfoProperty(wxPropertyGrid *pg, SoapySDR::ArgInfo arg) {
    
    wxPGProperty *prop = NULL;
    
    int intVal;
    double floatVal;
    std::vector<std::string>::iterator stringIter;
    
    switch (arg.type) {
        case SoapySDR::ArgInfo::INT:
            try {
                intVal = std::stoi(arg.value);
            } catch (std::invalid_argument e) {
                intVal = 0;
            }
            prop = pg->Append( new wxIntProperty(arg.name, wxPG_LABEL, intVal) );
            if (arg.range.minimum() != arg.range.maximum()) {
                pg->SetPropertyAttribute( prop, wxPG_ATTR_MIN, arg.range.minimum());
                pg->SetPropertyAttribute( prop, wxPG_ATTR_MAX, arg.range.maximum());
            }
            break;
        case SoapySDR::ArgInfo::FLOAT:
            try {
                floatVal = std::stod(arg.value);
            } catch (std::invalid_argument e) {
                floatVal = 0;
            }
            prop = pg->Append( new wxFloatProperty(arg.name, wxPG_LABEL, floatVal) );
            if (arg.range.minimum() != arg.range.maximum()) {
                pg->SetPropertyAttribute( prop, wxPG_ATTR_MIN, arg.range.minimum());
                pg->SetPropertyAttribute( prop, wxPG_ATTR_MAX, arg.range.maximum());
            }
            break;
        case SoapySDR::ArgInfo::BOOL:
            prop = pg->Append( new wxBoolProperty(arg.name, wxPG_LABEL, (arg.value=="true")) );
            break;
        case SoapySDR::ArgInfo::STRING:
            if (arg.options.size()) {
                intVal = 0;
                prop = pg->Append( new wxEnumProperty(arg.name, wxPG_LABEL) );
                for (stringIter = arg.options.begin(); stringIter != arg.options.end(); stringIter++) {
                    std::string optName = (*stringIter);
                    std::string displayName = optName;
                    if (arg.optionNames.size()) {
                        displayName = arg.optionNames[intVal];
                    }

                    prop->AddChoice(displayName);
                    if ((*stringIter)==arg.value) {
                        prop->SetChoiceSelection(intVal);
                    }
                    
                    intVal++;
                }
            } else {
                prop = pg->Append( new wxStringProperty(arg.name, wxPG_LABEL, arg.value) );
            }
            break;
    }
    
    if (prop != NULL) {
        prop->SetHelpString(arg.key + ": " + arg.description);
    }
    
    return prop;
}

void SDRDevicesDialog::OnSelectionChanged( wxTreeEvent& event ) {

    SDRDeviceInfo *selDev = getSelectedDevice(devTree->GetSelection());
    if (selDev) {
        dev = selDev;
        selId = devTree->GetSelection();
        DeviceConfig *devConfig = wxGetApp().getConfig()->getDevice(dev->getName());
        m_propertyGrid->Clear();

        SoapySDR::ArgInfoList args = dev->getSettingsArgInfo();
        SoapySDR::ArgInfoList::const_iterator args_i;

        m_propertyGrid->Append(new wxPropertyCategory("General Settings"));
        
        devSettings.erase(devSettings.begin(),devSettings.end());
        devSettings["name"] = m_propertyGrid->Append( new wxStringProperty("Name", wxPG_LABEL, devConfig->getDeviceName()) );
        devSettings["offset"] = m_propertyGrid->Append( new wxIntProperty("Offset (Hz)", wxPG_LABEL, devConfig->getOffset()) );
        
        if (args.size()) {
            m_propertyGrid->Append(new wxPropertyCategory("Run-time Settings"));
            
            
            for (args_i = args.begin(); args_i != args.end(); args_i++) {
                SoapySDR::ArgInfo arg = (*args_i);
                props.push_back(addArgInfoProperty(m_propertyGrid, arg));
            }
        }
        
        if (dev->getRxChannel()) {
            args = dev->getRxChannel()->getStreamArgsInfo();
            
            DeviceConfig *devConfig = wxGetApp().getConfig()->getDevice(dev->getDeviceId());
            ConfigSettings devStreamOpts = devConfig->getStreamOpts();
            if (devStreamOpts.size()) {
                for (int j = 0, jMax = args.size(); j < jMax; j++) {
                    if (devStreamOpts.find(args[j].key) != devStreamOpts.end()) {
                        args[j].value = devStreamOpts[args[j].key];
                    }
                }
            }

            if (args.size()) {
                m_propertyGrid->Append(new wxPropertyCategory("Stream Settings"));

                for (args_i = args.begin(); args_i != args.end(); args_i++) {
                    SoapySDR::ArgInfo arg = (*args_i);
                    props.push_back(addArgInfoProperty(m_propertyGrid, arg));
                }
            }
        }
        
    }
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

SDRDeviceInfo *SDRDevicesDialog::getSelectedDevice(wxTreeItemId selId) {
    devItems_i = devItems.find(selId);
    if (devItems_i != devItems.end()) {
        return devItems[selId];
    }
    return NULL;
}

void SDRDevicesDialog::OnUseSelected( wxMouseEvent& event ) {
    if (dev != NULL) {
        
        int i = 0;
        SoapySDR::ArgInfoList::const_iterator args_i;
        SoapySDR::ArgInfoList args = dev->getSettingsArgInfo();
        
        SoapySDR::Kwargs settingArgs;
        SoapySDR::Kwargs streamArgs;
        
        for (args_i = args.begin(); args_i != args.end(); args_i++) {
            SoapySDR::ArgInfo arg = (*args_i);
            wxPGProperty *prop = props[i];
            
            if (arg.type == SoapySDR::ArgInfo::STRING && arg.options.size()) {
                settingArgs[arg.key] = arg.options[prop->GetChoiceSelection()];
            } else if (arg.type == SoapySDR::ArgInfo::BOOL) {
                settingArgs[arg.key] = (prop->GetValueAsString()=="True")?"true":"false";
            } else {
                settingArgs[arg.key] = prop->GetValueAsString();
            }
            
            i++;
        }
        
        if (dev->getRxChannel()) {
            args = dev->getRxChannel()->getStreamArgsInfo();
            
            if (args.size()) {
                for (args_i = args.begin(); args_i != args.end(); args_i++) {
                    SoapySDR::ArgInfo arg = (*args_i);
                    wxPGProperty *prop = props[i];
            
                    if (arg.type == SoapySDR::ArgInfo::STRING && arg.options.size()) {
                        streamArgs[arg.key] = arg.options[prop->GetChoiceSelection()];
                    } else if (arg.type == SoapySDR::ArgInfo::BOOL) {
                        streamArgs[arg.key] = (prop->GetValueAsString()=="True")?"true":"false";
                    } else {
                        streamArgs[arg.key] = prop->GetValueAsString();
                    }
                    
                    i++;
                }
            }
        }
        
        AppConfig *cfg = wxGetApp().getConfig();
        DeviceConfig *devConfig = cfg->getDevice(dev->getDeviceId());
        devConfig->setSettings(settingArgs);
        devConfig->setStreamOpts(streamArgs);
        wxGetApp().setDeviceArgs(settingArgs);
        wxGetApp().setStreamArgs(streamArgs);
        wxGetApp().setDevice(dev);
        Close();
    }
}

void SDRDevicesDialog::OnTreeDoubleClick( wxMouseEvent& event ) {
    OnUseSelected(event);
}

void SDRDevicesDialog::OnDeviceTimer( wxTimerEvent& event ) {
    if (refresh) {
        if (wxGetApp().areModulesMissing()) {
            if (!failed) {
                wxMessageDialog *info;
                info = new wxMessageDialog(NULL, wxT("\nNo SoapySDR modules were found.\n\nCubicSDR requires at least one SoapySDR device support module to be installed.\n\nPlease visit https://github.com/cjcliffe/CubicSDR/wiki and in the build instructions for your platform read the 'Support Modules' section for more information."), wxT("\x28\u256F\xB0\u25A1\xB0\uFF09\u256F\uFE35\x20\u253B\u2501\u253B"), wxOK | wxICON_ERROR);
                info->ShowModal();
                failed = true;
            }
            return;
        }
        
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
        wxTreeItemId dsBranch = devTree->AppendItem(devRoot, "Local Net");
        wxTreeItemId remoteBranch = devTree->AppendItem(devRoot, "Remote");
        
        devs[""] = SDREnumerator::enumerate_devices("",true);
        if (devs[""] != NULL) {
            for (devs_i = devs[""]->begin(); devs_i != devs[""]->end(); devs_i++) {
                DeviceConfig *devConfig = wxGetApp().getConfig()->getDevice((*devs_i)->getDeviceId());
                if ((*devs_i)->isRemote()) {
                    devItems[devTree->AppendItem(dsBranch, devConfig->getDeviceName())] = (*devs_i);
                } else {
                    devItems[devTree->AppendItem(localBranch, devConfig->getDeviceName())] = (*devs_i);
                }
            }
        }
        
        std::vector<std::string> remotes = SDREnumerator::getRemotes();
        std::vector<std::string>::iterator remotes_i;
        std::vector<SDRDeviceInfo *>::iterator remoteDevs_i;
        
        if (remotes.size()) {
            for (remotes_i = remotes.begin(); remotes_i != remotes.end(); remotes_i++) {
                devs[*remotes_i] = SDREnumerator::enumerate_devices(*remotes_i, true);
                DeviceConfig *devConfig = wxGetApp().getConfig()->getDevice(*remotes_i);

                wxTreeItemId remoteNode = devTree->AppendItem(remoteBranch, devConfig->getDeviceName());
                
                if (devs[*remotes_i] != NULL) {
                    for (remoteDevs_i = devs[*remotes_i]->begin(); remoteDevs_i != devs[*remotes_i]->end(); remoteDevs_i++) {
                        devItems[devTree->AppendItem(remoteNode, (*remoteDevs_i)->getName())] = (*remoteDevs_i);
                    }
                }
            }
        }
        
        m_refreshButton->Enable();
        m_addRemoteButton->Enable();
        m_useSelectedButton->Enable();
        devTree->Enable();
        devTree->ExpandAll();
        
        devStatusBar->SetStatusText("Ready.");

        refresh = false;
    }
}

void SDRDevicesDialog::OnRefreshDevices( wxMouseEvent& event ) {
    wxGetApp().stopDevice();
    devTree->DeleteAllItems();
    devTree->Disable();
    m_propertyGrid->Clear();
    props.erase(props.begin(),props.end());
    devSettings.erase(devSettings.begin(), devSettings.end());
    m_refreshButton->Disable();
    m_addRemoteButton->Disable();
    m_useSelectedButton->Disable();
    wxGetApp().reEnumerateDevices();
    selId = 0;
    dev = nullptr;
    refresh = true;
}

void SDRDevicesDialog::OnPropGridChanged( wxPropertyGridEvent& event ) {
    if (dev && event.GetProperty() == devSettings["name"]) {
        DeviceConfig *devConfig = wxGetApp().getConfig()->getDevice(dev->getDeviceId());
        
        wxString devName = event.GetPropertyValue().GetString();
        
        devConfig->setDeviceName(devName.ToStdString());
        if (selId) {
            devTree->SetItemText(selId, devConfig->getDeviceName());
        }
        if (devName == "") {
            event.GetProperty()->SetValueFromString(devConfig->getDeviceName());
        }
    }
    if (dev && event.GetProperty() == devSettings["offset"]) {
        DeviceConfig *devConfig = wxGetApp().getConfig()->getDevice(dev->getDeviceId());
        
        long offset = event.GetPropertyValue().GetInteger();
        
        devConfig->setOffset(offset);
    }
}
