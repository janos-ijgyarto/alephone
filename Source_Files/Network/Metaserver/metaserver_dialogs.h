/*
 *  metaserver_dialogs.h - UI for metaserver client

	Copyright (C) 2004 and beyond by Woody Zenfell, III
	and the "Aleph One" developers.

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	This license is contained in the file "COPYING",
	which is included with this source code; it is available online at
	http://www.gnu.org/licenses/gpl.html

 April 29, 2004 (Woody Zenfell):
	Created.
 */

#ifndef METASERVER_DIALOGS_H
#define METASERVER_DIALOGS_H

#include "network_metaserver.h"
#include "metaserver_messages.h"
#include "shared_widgets.h"


const IPaddress run_network_metaserver_ui();

// This doesn't go here
void setupAndConnectClient(MetaserverClient& client);



struct game_info;

class GameAvailableMetaserverAnnouncer
{
public:
	GameAvailableMetaserverAnnouncer(const game_info& info);
	void Start();

private:
	// using gMetaserverClient instead
	// MetaserverClient	m_client;
};

class GlobalMetaserverChatNotificationAdapter : public MetaserverClient::NotificationAdapter
{
public:
	virtual void playersInRoomChanged(const std::vector<MetaserverPlayerInfo>&);
	virtual void gamesInRoomChanged(const std::vector<GameListMessage::GameListEntry>&);
	virtual void receivedChatMessage(const std::string& senderName, uint32 senderID, const std::string& message);
	virtual void receivedLocalMessage(const std::string& message);
	virtual void receivedBroadcastMessage(const std::string& message);
};

// Eventually this may disappear behind the facade of run_network_metaserver_ui()
// Or maybe it will disappear instead, leaving this.  Unsure.
class MetaserverClientUi : public GlobalMetaserverChatNotificationAdapter
{
public:
	// Abstract factory; concrete type determined at link-time
	static std::auto_ptr<MetaserverClientUi> Create();

	const IPaddress GetJoinAddressByRunning();

	virtual ~MetaserverClientUi () {};

protected:
	MetaserverClientUi() : m_used (false) {}

	void delete_widgets ();

	virtual void Run() = 0;
	virtual void Stop() = 0;

	void GameSelected(GameListMessage::GameListEntry game);
	void playersInRoomChanged(const std::vector<MetaserverPlayerInfo> &playerChanges);
	void gamesInRoomChanged(const std::vector<GameListMessage::GameListEntry> &gamesChanges);
	void sendChat();
	void ChatTextEntered (char character);
	void handleCancel();
	
	PlayerListWidget*				m_playersInRoomWidget;
	GameListWidget*					m_gamesInRoomWidget;
	EditTextWidget*					m_chatEntryWidget;
	HistoricTextboxWidget*				m_textboxWidget;
	ButtonWidget*					m_cancelWidget;
	IPaddress					m_joinAddress;
	bool						m_used;
};

#endif // METASERVER_DIALOGS_H
