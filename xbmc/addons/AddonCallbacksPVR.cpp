/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "AddonCallbacksPVR.h"

#include "utils/log.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "epg/Epg.h"
#include "pvr/PVRManager.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/timers/PVRTimers.h"
#include "pvr/addons/PVRClients.h"

using namespace PVR;
using namespace EPG;
using namespace ADDON;

CAddonCallbacksPVR::CAddonCallbacksPVR(CAddon* addon)
{
  m_addon     = addon;
  m_callbacks = new CB_PVRLib;

  /* write XBMC PVR specific add-on function addresses to callback table */
  m_callbacks->AllocateDemuxPacket        = PVRAllocateDemuxPacket;
  m_callbacks->FreeDemuxPacket            = PVRFreeDemuxPacket;
  m_callbacks->TransferChannelEntry       = PVRTransferChannelEntry;
  m_callbacks->TransferChannelGroup       = PVRTransferChannelGroup;
  m_callbacks->TransferChannelGroupMember = PVRTransferChannelGroupMember;
  m_callbacks->TransferEpgEntry           = PVRTransferEpgEntry;
  m_callbacks->TransferMenuHook           = PVRTransferMenuHook;
  m_callbacks->TransferRecordingEntry     = PVRTransferRecordingEntry;
  m_callbacks->TransferTimerEntry         = PVRTransferTimerEntry;
  m_callbacks->Recording                  = PVRRecording;
}

CAddonCallbacksPVR::~CAddonCallbacksPVR()
{
  /* delete the callback table */
  SAFE_DELETE(m_callbacks);
}

bool CAddonCallbacksPVR::GetPVRClient(void *addonData, PVR_CLIENT &client)
{
  CAddonCallbacks *addon = static_cast<CAddonCallbacks *>(addonData);
  if (!addon || !addon->GetHelperPVR() || !addon->GetHelperPVR()->m_addon)
  {
    CLog::Log(LOGERROR, "PVR - %s - called with a null pointer", __FUNCTION__);
    return false;
  }

  return g_PVRClients->GetClient(addon->GetHelperPVR()->m_addon->ID(), client);
}

void CAddonCallbacksPVR::PVRFreeDemuxPacket(void *addonData, DemuxPacket* pPacket)
{
  CDVDDemuxUtils::FreeDemuxPacket(pPacket);
}

DemuxPacket* CAddonCallbacksPVR::PVRAllocateDemuxPacket(void *addonData, int iDataSize)
{
  return CDVDDemuxUtils::AllocateDemuxPacket(iDataSize);
}

void CAddonCallbacksPVR::PVRTransferChannelEntry(void *addonData, const ADDON_HANDLE handle, const PVR_UPDATE_TYPE updateType, const PVR_CHANNEL *channel)
{
  PVR_CLIENT client;
  // validate input
  if (!addonData || !channel)
  {
    CLog::Log(LOGERROR, "PVR - %s - invalid data", __FUNCTION__);
  }
  // get the client
  else if (!GetPVRClient(addonData, client))
  {
    CLog::Log(LOGERROR, "PVR - %s - invalid PVR client", __FUNCTION__);
  }
  // only process responses when not fully initialised
  else if (!g_PVRManager.IsStarted() && updateType != PVR_UPDATE_RESPONSE)
  {
    return;
  }
  else
  {
    if (updateType != PVR_UPDATE_RESPONSE)
      CLog::Log(LOGDEBUG, "PVR - %s - %s channel '%s' (%d)", __FUNCTION__, ToString(updateType), channel->strChannelName, channel->iUniqueId);

    // get the internal channel group
    CPVRChannelGroupInternal *xbmcGroup(NULL);
    if (handle && handle->dataAddress)
      xbmcGroup = static_cast<CPVRChannelGroupInternal *>(handle->dataAddress);
    else
      xbmcGroup = g_PVRChannelGroups->GetGroupAll(channel->bIsRadio);

    // update
    if (!xbmcGroup)
      CLog::Log(LOGERROR, "PVR - %s - cannot find the channel group", __FUNCTION__);
    else
      xbmcGroup->UpdateFromClient(client, updateType, *channel);
  }
}

void CAddonCallbacksPVR::PVRTransferChannelGroup(void *addonData, const ADDON_HANDLE handle, const PVR_UPDATE_TYPE updateType, const PVR_CHANNEL_GROUP *group)
{
  PVR_CLIENT client;
  // validate input
  if (!addonData || !group)
  {
    CLog::Log(LOGERROR, "PVR - %s - invalid data", __FUNCTION__);
  }
  else if (strlen(group->strGroupName) == 0)
  {
    CLog::Log(LOGERROR, "PVR - %s - empty group name", __FUNCTION__);
  }
  // get the client
  else if (!GetPVRClient(addonData, client))
  {
    CLog::Log(LOGERROR, "PVR - %s - invalid PVR client", __FUNCTION__);
  }
  // only process responses when not fully initialised
  else if (!g_PVRManager.IsStarted() && updateType != PVR_UPDATE_RESPONSE)
  {
    return;
  }
  else
  {
    if (updateType != PVR_UPDATE_RESPONSE)
      CLog::Log(LOGDEBUG, "PVR - %s - %s group '%s'", __FUNCTION__, ToString(updateType), group->strGroupName);

    // get the groups container
    CPVRChannelGroups *xbmcGroups(NULL);
    if (handle && handle->dataAddress)
      xbmcGroups = static_cast<CPVRChannelGroups *>(handle->dataAddress);
    else
      xbmcGroups = g_PVRChannelGroups->Get(group->bIsRadio);

    // update
    if (!xbmcGroups)
      CLog::Log(LOGERROR, "PVR - %s - cannot find the groups container", __FUNCTION__);
    else
      xbmcGroups->UpdateFromClient(client, updateType, *group);
  }
}

void CAddonCallbacksPVR::PVRTransferChannelGroupMember(void *addonData, const ADDON_HANDLE handle, const PVR_UPDATE_TYPE updateType, const PVR_CHANNEL_GROUP_MEMBER *member)
{
  PVR_CLIENT client;
  // validate input
  if (!addonData || !member)
  {
    CLog::Log(LOGERROR, "PVR - %s - invalid data", __FUNCTION__);
  }
  else if (strlen(member->strGroupName) == 0)
  {
    CLog::Log(LOGERROR, "PVR - %s - empty group name", __FUNCTION__);
  }
  // get the client
  else if (!GetPVRClient(addonData, client))
  {
    CLog::Log(LOGERROR, "PVR - %s - invalid PVR client", __FUNCTION__);
  }
  // only process responses when not fully initialised
  else if (!g_PVRManager.IsStarted() && updateType != PVR_UPDATE_RESPONSE)
  {
    return;
  }
  else
  {
    if (updateType != PVR_UPDATE_RESPONSE)
      CLog::Log(LOGDEBUG, "PVR - %s - %s group member %d in group '%s'", __FUNCTION__, ToString(updateType), member->iChannelUniqueId, member->strGroupName);

    // get the group
    CPVRChannelGroup *xbmcGroup(NULL);
    if (handle && handle->dataAddress)
      xbmcGroup = static_cast<CPVRChannelGroup *>(handle->dataAddress);
    else
      xbmcGroup = g_PVRChannelGroups->GetByNameFromAll(member->strGroupName);

    // update
    if (!xbmcGroup)
      CLog::Log(LOGERROR, "PVR - %s - cannot find group '%s'", __FUNCTION__, member->strGroupName);
    else
      xbmcGroup->UpdateFromClient(client, updateType, *member);
  }
}

void CAddonCallbacksPVR::PVRTransferEpgEntry(void *addonData, const ADDON_HANDLE handle, const PVR_UPDATE_TYPE updateType, const EPG_TAG *epgentry)
{
  PVR_CLIENT client;
  // validate input
  if (!addonData || !epgentry)
  {
    CLog::Log(LOGERROR, "PVR - %s - invalid data", __FUNCTION__);
  }
  else if (strlen(epgentry->strTitle) == 0)
  {
    CLog::Log(LOGERROR, "PVR - %s - empty title", __FUNCTION__);
  }
  // get the client
  else if (!GetPVRClient(addonData, client))
  {
    CLog::Log(LOGERROR, "PVR - %s - invalid PVR client", __FUNCTION__);
  }
  // only process responses when not fully initialised
  else if (!g_PVRManager.IsStarted() && updateType != PVR_UPDATE_RESPONSE)
  {
    return;
  }
  else
  {
    if (updateType != PVR_UPDATE_RESPONSE)
      CLog::Log(LOGDEBUG, "PVR - %s - %s epg entry '%s'", __FUNCTION__, ToString(updateType), epgentry->strTitle);

    // get the epg container
    CEpg *xbmcEpg(NULL);
    if (handle && handle->dataAddress)
      xbmcEpg = static_cast<CEpg *>(handle->dataAddress);
    else
    {
      CPVRChannel *xbmcChannel = g_PVRChannelGroups->GetByUniqueID(epgentry->iChannelUniqueId, client->GetID());
      xbmcEpg = xbmcChannel ? xbmcChannel->GetEPG() : NULL;
    }

    // update
    if (!xbmcEpg)
      CLog::Log(LOGERROR, "PVR - %s - cannot find the epg table", __FUNCTION__);
    else
      xbmcEpg->UpdateFromClient(client, updateType, *epgentry);
  }
}

void CAddonCallbacksPVR::PVRTransferMenuHook(void *addonData, const ADDON_HANDLE handle, const PVR_UPDATE_TYPE updateType, const PVR_MENUHOOK *hook)
{
  PVR_CLIENT client;
  // validate input
  if (!addonData || !hook)
  {
    CLog::Log(LOGERROR, "PVR - %s - invalid data", __FUNCTION__);
  }
  // get the client
  else if (!GetPVRClient(addonData, client))
  {
    CLog::Log(LOGERROR, "PVR - %s - invalid PVR client", __FUNCTION__);
  }
  // only process responses when not fully initialised
  else if (!g_PVRManager.IsStarted() && updateType != PVR_UPDATE_RESPONSE)
  {
    return;
  }
  else
  {
    client->UpdateMenuHook(updateType, *hook);
  }
}

void CAddonCallbacksPVR::PVRTransferRecordingEntry(void *addonData, const ADDON_HANDLE handle, const PVR_UPDATE_TYPE updateType, const PVR_RECORDING *recording)
{
  PVR_CLIENT client;
  // validate input
  if (!addonData || !recording)
  {
    CLog::Log(LOGERROR, "PVR - %s - invalid data", __FUNCTION__);
  }
  else if (strlen(recording->strTitle) == 0)
  {
    CLog::Log(LOGERROR, "PVR - %s - empty title", __FUNCTION__);
  }
  else if (strlen(recording->strStreamURL) == 0)
  {
    CLog::Log(LOGERROR, "PVR - %s - empty stream url", __FUNCTION__);
  }
  else if (strlen(recording->strRecordingId) == 0)
  {
    CLog::Log(LOGERROR, "PVR - %s - empty recording id", __FUNCTION__);
  }
  // get the client
  else if (!GetPVRClient(addonData, client))
  {
    CLog::Log(LOGERROR, "PVR - %s - invalid PVR client", __FUNCTION__);
  }
  // only process responses when not fully initialised
  else if (!g_PVRManager.IsStarted() && updateType != PVR_UPDATE_RESPONSE)
  {
    return;
  }
  else
  {
    if (updateType != PVR_UPDATE_RESPONSE)
      CLog::Log(LOGDEBUG, "PVR - %s - %s recording '%s'", __FUNCTION__, ToString(updateType), recording->strTitle);

    // get the recordings container
    CPVRRecordings *xbmcRecordings(NULL);
    if (handle && handle->dataAddress)
      xbmcRecordings = static_cast<CPVRRecordings *>(handle->dataAddress);
    else
      xbmcRecordings = g_PVRRecordings;

    // update
    if (!xbmcRecordings)
      CLog::Log(LOGERROR, "PVR - %s - cannot find the recordings container", __FUNCTION__);
    else
      xbmcRecordings->UpdateFromClient(client, updateType, *recording);
  }
}

void CAddonCallbacksPVR::PVRTransferTimerEntry(void *addonData, const ADDON_HANDLE handle, const PVR_UPDATE_TYPE updateType, const PVR_TIMER *timer)
{
  PVR_CLIENT client;
  // validate input
  if (!addonData || !timer)
  {
    CLog::Log(LOGERROR, "PVR - %s - invalid data", __FUNCTION__);
  }
  // get the client
  else if (!GetPVRClient(addonData, client))
  {
    CLog::Log(LOGERROR, "PVR - %s - invalid PVR client", __FUNCTION__);
  }
  // only process responses when not fully initialised
  else if (!g_PVRManager.IsStarted() && updateType != PVR_UPDATE_RESPONSE)
  {
    return;
  }
  else
  {
    if (updateType != PVR_UPDATE_RESPONSE)
      CLog::Log(LOGDEBUG, "PVR - %s - %s timer '%s'", __FUNCTION__, ToString(updateType), timer->strTitle);

    // get the timers container
    CPVRTimers *xbmcTimers(NULL);
    if (handle && handle->dataAddress)
      xbmcTimers = static_cast<CPVRTimers *>(handle->dataAddress);
    else
      xbmcTimers = g_PVRTimers;

    // update
    if (!xbmcTimers)
      CLog::Log(LOGERROR, "PVR - %s - cannot find the timers container", __FUNCTION__);
    else
      xbmcTimers->UpdateFromClient(client, updateType, *timer);
  }
}

void CAddonCallbacksPVR::PVRRecording(void *addonData, const char *strName, const char *strFileName, bool bOnOff)
{
  PVR_CLIENT client;
  // validate input
  if (!addonData || !strName || !strFileName)
  {
    CLog::Log(LOGERROR, "PVR - %s - invalid data", __FUNCTION__);
  }
  // get the client
  else if (!GetPVRClient(addonData, client))
  {
    CLog::Log(LOGERROR, "PVR - %s - invalid PVR client", __FUNCTION__);
  }
  else
  {
    CStdString strLine1;
    if (bOnOff)
      strLine1.Format(g_localizeStrings.Get(19197), client->Name());
    else
      strLine1.Format(g_localizeStrings.Get(19198), client->Name());

    CStdString strLine2;
    if (strName)
      strLine2 = strName;
    else if (strFileName)
      strLine2 = strFileName;

    /* display a notification for 5 seconds */
    CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, strLine1, strLine2, 5000, false);

    CLog::Log(LOGDEBUG, "CAddonCallbacksPVR - %s - recording %s on client '%s'. name='%s' filename='%s'",
        __FUNCTION__, bOnOff ? "started" : "finished", client->Name().c_str(), strName, strFileName);
  }
}

const char *CAddonCallbacksPVR::ToString(const PVR_UPDATE_TYPE updateType)
{
  switch (updateType)
  {
  case PVR_UPDATE_DELETE:
    return "delete";
  case PVR_UPDATE_NEW:
    return "new";
  case PVR_UPDATE_REPLACE:
    return "replace";
  case PVR_UPDATE_RESPONSE:
    return "response";
  default:
    return "unknown";
  }
}
