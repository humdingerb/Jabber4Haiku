//////////////////////////////////////////////////
// Blabber [TalkWindow.cpp]
//////////////////////////////////////////////////

#ifndef __CSTDIO__
	#include <cstdio>
#endif

#ifndef _CTIME
	#include <ctime>
#endif

#ifndef _BOX_H
	#include <interface/Box.h>
#endif

#ifndef _NETPOSITIVE_H
	#include <be_apps/NetPositive/NetPositive.h>
#endif

#ifndef _ROSTER_H
	#include <Roster.h>
#endif

#ifndef _PATH_H
	#include <storage/Path.h>
#endif

#ifndef APP_LOCATION_H
	#include "AppLocation.h"
#endif

#ifndef BLABBER_SETTINGS_H
	#include "BlabberSettings.h"
#endif

#ifndef COMMAND_MESSAGE_H
	#include "CommandMessage.h"
#endif

#ifndef GENERIC_FUNCTIONS_H
	#include "GenericFunctions.h"
#endif

#ifndef JABBER_SPEAK_H
	#include "JabberSpeak.h"
#endif

#ifndef MESSAGE_REPEATER_H
	#include "MessageRepeater.h"
#endif

#ifndef MESSAGES_H
	#include "Messages.h"
#endif

#ifndef PEOPLE_LIST_ITEM_H
	#include "PeopleListItem.h"
#endif

#ifndef PREFERENCES_WINDOW_H
	#include "PreferencesWindow.h"
#endif

#ifndef ROTATE_CHAT_FILTER_H
	#include "RotateChatFilter.h"
#endif

#ifndef SOUND_SYSTEM_H
	#include "SoundSystem.h"
#endif

#ifndef TALK_LIST_ITEM_H
	#include "TalkListItem.h"
#endif

#ifndef TALK_MANAGER_H
	#include "TalkManager.h"
#endif

#ifndef TALK_WINDOW_H
	#include "TalkWindow.h"
#endif

float TalkWindow::x_placement_offset = -100;
float TalkWindow::y_placement_offset = -100;

TalkWindow::TalkWindow(talk_type type, const UserID *user, string group_room, string group_username, bool follow_focus_rules)
	: BWindow(BRect(0, 0, 0, 0), "<talk window>", B_DOCUMENT_WINDOW, B_AVOID_FOCUS) {
	_am_logging = false;
	_log        = NULL;
	_chat_index = -1;
	
	// add self to message family
	MessageRepeater::Instance()->AddTarget(this);

	_type           = type;
	_user           = user;
	_group_room     = group_room;
	_group_username = group_username;

	if (_type != TalkWindow::GROUP) {
		_current_status = user->OnlineStatus();
	}

	// set size constraints
	SetSizeLimits(272, 3000, 220, 3000);
	
	// generate a thread ID
	_thread = GenericFunctions::GenerateUniqueID();
	
	// determine window size
	BRect rect;

	float login_window_width;
	float login_window_height;

	if (_type == TalkWindow::MESSAGE && BlabberSettings::Instance()->Data("message-window-width")) {
		login_window_width = atof(BlabberSettings::Instance()->Data("message-window-width"));
	} else {
		if (_type == TalkWindow::GROUP) {
			login_window_width = 500;
		} else {
			login_window_width = 272;
		}
	}
	
	if (_type == TalkWindow::MESSAGE && BlabberSettings::Instance()->Data("message-window-height")) {
		login_window_height = atof(BlabberSettings::Instance()->Data("message-window-height"));
	} else {
		if (_type == TalkWindow::GROUP) {
			login_window_height = 400;
		} else {
			login_window_height = 279;
		}
	}

	// create window frame position
	rect = GenericFunctions::CenteredFrame(login_window_width, login_window_height);

	// calculate based on offsets and adjust
	rect.OffsetBy(x_placement_offset, y_placement_offset);

	if (x_placement_offset < 100.0) {
		x_placement_offset += 25.0;
	} else {
		x_placement_offset = -100.0;
	}

	if (y_placement_offset < 100.0) {
		y_placement_offset += 25.0;
	} else {
		y_placement_offset = -100.0;
	}

	// set it
	ResizeTo(rect.Width(), rect.Height());
	MoveTo(rect.LeftTop());
	
	// encompassing view
	rect = Bounds();
	rect.OffsetTo(B_ORIGIN);

	_full_view = new BView(rect, "main-full", B_FOLLOW_ALL, B_WILL_DRAW);
	_full_view->SetViewColor(216, 216, 216, 255);

	rect = Bounds();

	// menubar (top of buddy window)
	rect.bottom = rect.top + 18;

	_menubar = new BMenuBar(rect, "menubar");

	// FILE MENU
	_file_menu = new BMenu("File");

		_record_item = new BMenuItem("Start Chat Log", new BMessage(JAB_START_RECORD));
		_record_item->SetEnabled(true);
		_record_entire_item = new BMenuItem("Stop Chat Log", new BMessage(JAB_STOP_RECORD));
		_record_entire_item->SetEnabled(false);
		_close_item = new BMenuItem("Close", new BMessage(JAB_CLOSE_CHAT));
		_close_item->SetShortcut('W', 0);

	if (_type == CHAT) {
		_file_menu->AddItem(_record_item);
		_file_menu->AddItem(_record_entire_item);
		_file_menu->AddSeparatorItem();
	}
		
	_file_menu->AddItem(_close_item);
	_file_menu->SetTargetForItems(this);

	// EDIT MENU
	_edit_menu = new BMenu("Edit");

		_copy_item = new BMenuItem("Cut", new BMessage(B_CUT));
		_copy_item->SetShortcut('C', 0);

		_cut_item = new BMenuItem("Copy", new BMessage(B_COPY));
		_cut_item->SetShortcut('X', 0);

		_paste_item = new BMenuItem("Paste", new BMessage(B_PASTE));
		_paste_item->SetShortcut('V', 0);

		_select_all_item = new BMenuItem("Select All", new BMessage(B_SELECT_ALL));
		_select_all_item->SetShortcut('A', 0);

		_preferences_item = new BMenuItem("Preferences...", new BMessage(JAB_PREFERENCES));

//	_edit_menu->AddItem(_cut_item);
//	_edit_menu->AddItem(_copy_item);
//	_edit_menu->AddItem(_paste_item);
//	_edit_menu->AddItem(_select_all_item);
//	_edit_menu->AddSeparatorItem();
	_edit_menu->AddItem(_preferences_item);

	// MESSAGE MENU
	_message_menu = new BMenu("Messages");

		_message_1_item = new BMenuItem("Message #1", new BMessage(JAB_MESSAGE_1));
		_message_1_item->SetShortcut('1', 0);
		_message_1_item->SetEnabled(false);

		_message_2_item = new BMenuItem("Message #2", new BMessage(JAB_MESSAGE_2));
		_message_2_item->SetShortcut('2', 0);
		_message_2_item->SetEnabled(false);

		_message_3_item = new BMenuItem("Message #3", new BMessage(JAB_MESSAGE_3));
		_message_3_item->SetShortcut('3', 0);
		_message_3_item->SetEnabled(false);

		_message_4_item = new BMenuItem("Message #4", new BMessage(JAB_MESSAGE_4));
		_message_4_item->SetShortcut('4', 0);
		_message_4_item->SetEnabled(false);

		_message_5_item = new BMenuItem("Message #5", new BMessage(JAB_MESSAGE_5));
		_message_5_item->SetShortcut('5', 0);
		_message_5_item->SetEnabled(false);

		_message_6_item = new BMenuItem("Message #6", new BMessage(JAB_MESSAGE_6));
		_message_6_item->SetShortcut('6', 0);
		_message_6_item->SetEnabled(false);

		_message_7_item = new BMenuItem("Message #7", new BMessage(JAB_MESSAGE_7));
		_message_7_item->SetShortcut('7', 0);
		_message_7_item->SetEnabled(false);

		_message_8_item = new BMenuItem("Message #8", new BMessage(JAB_MESSAGE_8));
		_message_8_item->SetShortcut('8', 0);
		_message_8_item->SetEnabled(false);
		
		_message_9_item = new BMenuItem("Message #9", new BMessage(JAB_MESSAGE_9));
		_message_9_item->SetShortcut('9', 0);
		_message_9_item->SetEnabled(false);

	_message_menu->AddItem(_message_1_item);
	_message_menu->AddItem(_message_2_item);
	_message_menu->AddItem(_message_3_item);
	_message_menu->AddItem(_message_4_item);
	_message_menu->AddItem(_message_5_item);
	_message_menu->AddItem(_message_6_item);
	_message_menu->AddItem(_message_7_item);
	_message_menu->AddItem(_message_8_item);
	_message_menu->AddItem(_message_9_item);
	_message_menu->SetTargetForItems(this);

	// TALK MENU
	_talk_menu = new BMenu("Talk");

		_rotate_chat_forward_item = new BMenuItem("Rotate Chat Forward", new BMessage(JAB_ROTATE_CHAT_FORWARD));
		_rotate_chat_forward_item->SetShortcut('.', 0);

		_rotate_chat_backward_item = new BMenuItem("Rotate Chat Backward", new BMessage(JAB_ROTATE_CHAT_BACKWARD));
		_rotate_chat_backward_item->SetShortcut(',', 0);

		_rotate_buddy_list_item = new BMenuItem("Buddy List", new BMessage(JAB_FOCUS_BUDDY));
		_rotate_buddy_list_item->SetShortcut('/', 0);

	_talk_menu->AddItem(_rotate_chat_forward_item);
	_talk_menu->AddItem(_rotate_chat_backward_item);
	_talk_menu->AddSeparatorItem();
	_talk_menu->AddItem(_rotate_buddy_list_item);
	_talk_menu->SetTargetForItems(this);

	// HELP MENU
	_help_menu = new BMenu("Help");

		_jabber_org_item = new BMenuItem("Jabber.org", new BMessage(JAB_JABBER_ORG));
		_riv_item = new BMenuItem("OsDrawer Web site", new BMessage(JAB_RIV));
		_jabber_central_org_item = new BMenuItem("JabberCentral.org", new BMessage(JAB_JABBER_CENTRAL_ORG));
		_jabber_view_com_item = new BMenuItem("JabberView.com", new BMessage(JAB_JABBER_VIEW_COM));
		_user_guide_item = new BMenuItem("Jabber for Haiku Manual", new BMessage(JAB_USER_GUIDE));
		_faq_item = new BMenuItem("Jabber for Haiku FAQ", new BMessage(JAB_FAQ));
		BMessage* haiku_channel = new BMessage(JAB_A_CHANNEL);
		haiku_channel->AddString("channel", "haiku-os@conference.jabber.org");
		_beos_user_item = new BMenuItem("Haiku Users Directory", haiku_channel);

	_help_menu->AddItem(_user_guide_item);
	_help_menu->AddItem(_faq_item);
	_help_menu->AddItem(_beos_user_item);
	_help_menu->AddSeparatorItem();
	_help_menu->AddItem(_riv_item);
	_help_menu->AddSeparatorItem();
	_help_menu->AddItem(_jabber_org_item);
	_help_menu->AddItem(_jabber_central_org_item);
	_help_menu->AddItem(_jabber_view_com_item);
	_help_menu->SetTargetForItems(this);

	_menubar->AddItem(_file_menu);
	_menubar->AddItem(_edit_menu);
	_menubar->AddItem(_message_menu);
	_menubar->AddItem(_talk_menu);
	_menubar->AddItem(_help_menu);

	// status bar
	_status_view = new StatusView();
	_status_view->SetViewColor(216, 216, 216, 255);
	_status_view->SetLowColor(216, 216, 216, 255);

	// chat controls
	rect.top = rect.bottom + 1;
	rect.bottom = Bounds().bottom;
	rect.InsetBy(5.0, 5.0);
	rect.bottom -= 140.0;
	
	// conversation control
	rect.right -= B_V_SCROLL_BAR_WIDTH;

	BRect text_rect(rect);
	text_rect.OffsetTo(B_ORIGIN);
	text_rect.InsetBy(2.0, 2.0);
	
	_chat          = new ChatTextView(rect, "chat", text_rect, B_FOLLOW_ALL, B_WILL_DRAW | B_FRAME_EVENTS);
	_chat_scroller = new BScrollView("chat_scroller", _chat, B_FOLLOW_ALL, B_WILL_DRAW, false, true);
	_chat->TargetedByScrollView(_chat_scroller);
	_chat->SetFontSize(12.0);
	_chat->SetWordWrap(true);
	_chat->SetStylable(true);
	_chat->MakeEditable(false);

	// box
	rect.OffsetBy(-1.0, rect.Height() + 4.0);
	rect.bottom = Bounds().bottom - 5.0 - _status_view->GetHeight();
	rect.right += B_V_SCROLL_BAR_WIDTH + 3.0;
	rect.OffsetTo(1.0, 1.0);
	
	_sending = new BBox(rect, NULL, B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP_BOTTOM);
	_sending->SetLabel("Communicate");
		
	// message control
	rect.OffsetTo(B_ORIGIN);
	rect.InsetBy(6.0, 16.0);
	rect.bottom = rect.top + 62.0;
	
	text_rect = rect;
	text_rect.OffsetTo(B_ORIGIN);
	text_rect.InsetBy(2.0, 2.0);

	rgb_color sent = {0, 0, 0, 255};

	BFont black_11(be_plain_font);
	black_11.SetSize(11.0);
	
	_message          = new BetterTextView(rect, "message", text_rect, &black_11, &sent, B_FOLLOW_TOP_BOTTOM | B_FOLLOW_LEFT_RIGHT, B_WILL_DRAW);
	_message_scroller = new BScrollView("message_scroller", _message, B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP_BOTTOM, B_WILL_DRAW, false, false);
	_message->TargetedByScrollView(_message_scroller);
	_message->SetWordWrap(true);

	// editing filter for messaging
	AddCommonFilter(new EditingFilter(_message, this));

	// editing filter for taksing
	AddCommonFilter(new RotateChatFilter(this));

	// send button
	rect.OffsetBy(_message_scroller->Bounds().Width() - 65.0, _message_scroller->Bounds().Height() + 7.0);
	rect.right = rect.left + 50;
	rect.bottom = rect.top + 18.0;

	_send_message = new BButton(rect, "send", "Send", new BMessage(JAB_CHAT_SENT), B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM);
	_send_message->MakeDefault(true);
	_send_message->SetTarget(this);
	
	// add alt-enter note
	rect.left = 3.0;
	rect.right = 180.0;
	rect.bottom = rect.top + 18.0;
	rect.OffsetBy(0.0, -7.0);

	_command_enter = new BCheckBox(rect, NULL, "Newlines allowed?", new BMessage(NEWLINE_TOGGLE), B_FOLLOW_BOTTOM);
	_command_enter->SetValue(BlabberSettings::Instance()->Tag("last-command-enter"));
	
	rect.OffsetBy(0.0, 17.0);
	rect.bottom += 5.0;
	
	text_rect = rect;
	text_rect.OffsetTo(B_ORIGIN);
	
	rgb_color note = {0, 0, 0, 255};
	BFont black_8(be_plain_font);
	black_8.SetSize(8.0);

	_enter_note = new BTextView(rect, NULL, text_rect, &black_8, &note, B_FOLLOW_LEFT | B_FOLLOW_BOTTOM, B_WILL_DRAW);
	_enter_note->SetViewColor(216, 216, 216, 255);
	_enter_note->MakeEditable(false);
	_enter_note->MakeSelectable(false);

	// post message
	PostMessage(NEWLINE_TOGGLE);

	_sending->AddChild(_message_scroller);
	_sending->AddChild(_send_message);
	_sending->AddChild(_enter_note);
	_sending->AddChild(_command_enter);
	
	// handle splits
	rect = _sending->Bounds();
	rect.InsetBy(-3.0, -3.0);
	
	BView *chatting_view = new BView(rect, NULL, B_FOLLOW_ALL, B_WILL_DRAW);

	chatting_view->AddChild(_sending);
	chatting_view->SetViewColor(216, 216, 216);
	
	rect = Bounds();
		rect.top = 22.0;
		rect.bottom -= _status_view->GetHeight() + 3.0;
	
	if (_type == GROUP) {
		rect.right -= 80.0;
	}
	
	_split_talk = new SplitPane(rect, _chat_scroller, chatting_view, B_FOLLOW_TOP_BOTTOM | B_FOLLOW_LEFT_RIGHT);
	_split_talk->SetAlignment(B_HORIZONTAL);
	_split_talk->SetMinSizeTwo(_sending->Bounds().Height() + 2.0);
	_split_talk->SetBarThickness(6.0);
	_split_talk->SetBarAlignmentLocked(true);
	_split_talk->SetViewInsetBy(2.0);
	
	if (_type == GROUP) {
		rect = Bounds();
			rect.top = 22.0;
			rect.bottom -= _status_view->GetHeight() + 3.0;

		BRect people_rect = Bounds();
			people_rect.left = rect.right + 1.0;

		_people = new BListView(people_rect, NULL, B_SINGLE_SELECTION_LIST, B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP_BOTTOM);
		_scrolled_people_pane = new BScrollView(NULL, _people, B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP_BOTTOM, 0, false, true, B_PLAIN_BORDER);

		_split_group_people = new SplitPane(rect, _split_talk, _scrolled_people_pane, B_FOLLOW_ALL);
		_split_group_people->SetAlignment(B_VERTICAL);
		_split_group_people->SetMinSizeOne(300.0);
		_split_group_people->SetMinSizeTwo(100.0);
		_split_group_people->SetBarThickness(6.0);
		_split_group_people->SetBarAlignmentLocked(true);
		_split_group_people->SetBarPosition(400);
	}	
	
	// add GUI components to BView
	AddChild(_full_view);

	_full_view->AddChild(_menubar);
	_full_view->AddChild(_status_view);

	if (_type == GROUP) {
		_full_view->AddChild(_split_group_people);
	} else {
		_full_view->AddChild(_split_talk);
	}
	
	_message->MakeFocus(true);
	
	// generate window title
	char buffer[1024];
	string user_representation;
	
	if (_type == TalkWindow::GROUP) {
		// identify the user
		sprintf(buffer, "your identity is %s", _group_username.c_str());
		_status_view->SetMessage(buffer); 
	} else if (user->UserType() == UserID::JABBER) {
		// identify the user
		sprintf(buffer, "your identity is %s", JabberSpeak::Instance()->CurrentLogin().c_str());
		_status_view->SetMessage(buffer); 
	} else {
		user_representation = user->FriendlyName();
		
		if (user_representation.empty()) {
			user_representation = user->JabberUsername();
		}

		if (user->UserType() == UserID::AIM) {
			user_representation += " (AOL)";

			// identify the user
			if (!AgentList::Instance()->GetAgentByService("aim")->Username().empty()) {
				sprintf(buffer, "your identity is %s", AgentList::Instance()->GetAgentByService("aim")->Username().c_str());
				_status_view->SetMessage(buffer); 
			} else {
				sprintf(buffer, "you are communicating via the AOL service");
				_status_view->SetMessage(buffer); 
			}
		} else if (user->UserType() == UserID::ICQ) {
			user_representation += " (ICQ)";

			// identify the user
			if (!AgentList::Instance()->GetAgentByService("icq")->Username().empty()) {
				sprintf(buffer, "your identity is %s", AgentList::Instance()->GetAgentByService("icq")->Username().c_str());
				_status_view->SetMessage(buffer); 
			} else {
				sprintf(buffer, "you are communicating via the ICQ service");
				_status_view->SetMessage(buffer); 
			}
		} else if (user->UserType() == UserID::YAHOO) {
			user_representation += " (Yahoo!)";

			// identify the user
			if (!AgentList::Instance()->GetAgentByService("yahoo")->Username().empty()) {
				sprintf(buffer, "your identity is %s", AgentList::Instance()->GetAgentByService("yahoo")->Username().c_str());
				_status_view->SetMessage(buffer); 
			} else {
				sprintf(buffer, "you are communicating via the Yahoo! Messenger service");
				_status_view->SetMessage(buffer); 
			}
		} else if (user->UserType() == UserID::MSN) {
			user_representation += " (MSN)";

			// identify the user
			if (!AgentList::Instance()->GetAgentByService("msn")->Username().empty()) {
				sprintf(buffer, "your identity is %s", AgentList::Instance()->GetAgentByService("msn")->Username().c_str());
				_status_view->SetMessage(buffer); 
			} else {
				sprintf(buffer, "you are communicating via the MSN Messenger service");
				_status_view->SetMessage(buffer); 
			}
		}
	}

	if (_type != GROUP && user_representation.empty()) {
		user_representation = user->FriendlyName();
	}

	if (type == MESSAGE) {
		sprintf(buffer, "[message] %s", user_representation.c_str()); 
	} else if (type == CHAT) {
		sprintf(buffer, "[chat] %s", user_representation.c_str()); 
	} else if (type == GROUP) {
		sprintf(buffer, "[groupchat] %s", group_room.c_str()); 
	}
		
	SetTitle(buffer);

	// do this to allow the following...
	Show();
	
	// reset flags
	SetFlags(Flags() & ~B_AVOID_FOCUS);

	// focus rules
	if (!follow_focus_rules || !BlabberSettings::Instance()->Tag("suppress-chat-focus")) {
		Activate();
	}
}

TalkWindow::~TalkWindow() {
	// close log cleanly if it's open
	if (_log) {
		fclose(_log);
	}

	delete _user;
	
	// remove self from message family
	MessageRepeater::Instance()->RemoveTarget(this);
}

TalkWindow::talk_type TalkWindow::Type() {
	return _type;
}

void TalkWindow::FrameResized(float width, float height) {
	BWindow::FrameResized(width, height);

	BRect chat_rect    = _chat->Frame();
	BRect message_rect = _message->Frame();

	chat_rect.OffsetTo(B_ORIGIN);
	message_rect.OffsetTo(B_ORIGIN);
	
	chat_rect.InsetBy(2.0, 2.0);
	message_rect.InsetBy(2.0, 2.0);
	
	_chat->SetTextRect(chat_rect);
	_message->SetTextRect(message_rect);

	_chat->Invalidate();
	_chat_scroller->Invalidate();
	
	// remember sizes of message windows
	if (_type == TalkWindow::MESSAGE) {
		BlabberSettings::Instance()->SetFloatData("message-window-width", width);
		BlabberSettings::Instance()->SetFloatData("message-window-height", height);
	}
}

void TalkWindow::MenusBeginning() {
	const char *message_1 = BlabberSettings::Instance()->Data("message-1");
	const char *message_2 = BlabberSettings::Instance()->Data("message-2");
	const char *message_3 = BlabberSettings::Instance()->Data("message-3");
	const char *message_4 = BlabberSettings::Instance()->Data("message-4");
	const char *message_5 = BlabberSettings::Instance()->Data("message-5");
	const char *message_6 = BlabberSettings::Instance()->Data("message-6");
	const char *message_7 = BlabberSettings::Instance()->Data("message-7");
	const char *message_8 = BlabberSettings::Instance()->Data("message-8");
	const char *message_9 = BlabberSettings::Instance()->Data("message-9");
	
	if (message_1) {
		_message_1_item->SetLabel(message_1);
		_message_1_item->SetEnabled(true);
	} else {
		_message_1_item->SetLabel("Message 1");
		_message_1_item->SetEnabled(false);
	}

	if (message_2) {
		_message_2_item->SetLabel(message_2);
		_message_2_item->SetEnabled(true);
	} else {
		_message_2_item->SetLabel("Message 2");
		_message_2_item->SetEnabled(false);
	}

	if (message_3) {
		_message_3_item->SetLabel(message_3);
		_message_3_item->SetEnabled(true);
	} else {
		_message_3_item->SetLabel("Message 3");
		_message_3_item->SetEnabled(false);
	}

	if (message_4) {
		_message_4_item->SetLabel(message_4);
		_message_4_item->SetEnabled(true);
	} else {
		_message_4_item->SetLabel("Message 4");
		_message_4_item->SetEnabled(false);
	}

	if (message_5) {
		_message_5_item->SetLabel(message_5);
		_message_5_item->SetEnabled(true);
	} else {
		_message_5_item->SetLabel("Message 5");
		_message_5_item->SetEnabled(false);
	}

	if (message_6) {
		_message_6_item->SetLabel(message_6);
		_message_6_item->SetEnabled(true);
	} else {
		_message_6_item->SetLabel("Message 6");
		_message_6_item->SetEnabled(false);
	}

	if (message_7) {
		_message_7_item->SetLabel(message_7);
		_message_7_item->SetEnabled(true);
	} else {
		_message_7_item->SetLabel("Message 7");
		_message_7_item->SetEnabled(false);
	}

	if (message_8) {
		_message_8_item->SetLabel(message_8);
		_message_8_item->SetEnabled(true);
	} else {
		_message_8_item->SetLabel("Message 8");
		_message_8_item->SetEnabled(false);
	}

	if (message_9) {
		_message_9_item->SetLabel(message_9);
		_message_9_item->SetEnabled(true);
	} else {
		_message_9_item->SetLabel("Message 9");
		_message_9_item->SetEnabled(false);
	}

	// logging
	if (_am_logging) {
		_record_item->SetEnabled(false);
		_record_entire_item->SetEnabled(true);
	} else {
		_record_item->SetEnabled(true);
		_record_entire_item->SetEnabled(false);
	}

}

void TalkWindow::MessageReceived(BMessage *msg) {
	switch(msg->what) {
		case JAB_CLOSE_TALKS: {
			PostMessage(B_QUIT_REQUESTED);
			break;
		}

		case JAB_GROUP_CHATTER_ONLINE: {
			// only for groupchat
			if (_type != GROUP) {
				break;
			}

			if (GetGroupRoom() == msg->FindString("room")) {
				AddGroupChatter(msg->FindString("username"));
			}
			
			break;
		}

		case JAB_GROUP_CHATTER_OFFLINE: {
			// only for groupchat
			if (_type != GROUP) {
				break;
			}

			RemoveGroupChatter(msg->FindString("username"));
			
			break;
		}
		
		case JAB_PREFERENCES: {
			PreferencesWindow::Instance()->Show();
			PreferencesWindow::Instance()->Activate();
			break;
		}
		
		case JAB_START_RECORD: {
			if (_am_logging) {
				// we already are
				break;
			}
			
			// just open file panel for now
			_fp = new BFilePanel(B_SAVE_PANEL, new BMessenger(this, this), NULL, 0, false, NULL);
			_fp->Show();
			
			break;
		}

		case B_SAVE_REQUESTED: {
			// collect data
			char abs_path[B_PATH_NAME_LENGTH];

			entry_ref dir;
			string filename;

			msg->FindRef("directory", &dir);
			filename = msg->FindString("name");			

			// construct path (sigh)
			BDirectory dir_object(&dir);
			
			strcpy(abs_path, BPath(&dir_object, filename.c_str()).Path());
			
			_am_logging = true;
			
			// start file
			_log = fopen(abs_path, "w");

			// log existing data
			Log(_chat->Text());
						
			break;
		}

		case JAB_STOP_RECORD: {
			if (!_am_logging) {
				// we're already not :-)
				break;
			}

			// dirty work
			_am_logging = false;
			fclose(_log);
			
			break;
		}
		
		case NEWLINE_TOGGLE: {
			if (NewlinesAllowed()) {
				_enter_note->SetText("Enter adds newline; Command-Enter sends message.");
			} else {
				_enter_note->SetText("Command-Enter adds newline; Enter sends message.");
			}
			
			// save last value
			BlabberSettings::Instance()->SetTag("last-command-enter", NewlinesAllowed());
			BlabberSettings::Instance()->WriteToFile();
			
			break;
		}
		
		case BLAB_UPDATE_ROSTER: {
			// doesn't apply to groupchat
			if (_type == GROUP) {
				break;
			}

			JRoster::Instance()->Lock();

			// get REAL pointer to existing user
			UserID *user = JRoster::Instance()->FindUser(JRoster::HANDLE, _user->JabberHandle());

			// get new status
			if (user == NULL) {
				// may have been removed?
				JRoster::Instance()->Unlock();
				break;
			}
			
			UserID::online_status new_status = user->OnlineStatus();

			// if we have one, check their presence			
			if (_current_status != new_status) {
				char buffer[2048];				

				if (_current_status == UserID::ONLINE && new_status == UserID::OFFLINE) {
					sprintf(buffer, "This user is now offline.");
					AddToTalk("", buffer, OTHER);
				} else if (_current_status == UserID::OFFLINE && new_status == UserID::ONLINE) {
					sprintf(buffer, "This user is now online.");
					AddToTalk("", buffer, OTHER);
				}
			}

			_current_status = new_status;

			JRoster::Instance()->Unlock();
		
			break;
		}
		
		case JAB_MESSAGE_1: {
			string message = BlabberSettings::Instance()->Data("message-1");
			
			if (!message.empty() && BlabberSettings::Instance()->Tag("fire-1")) {
				// network part
				if (_type == TalkWindow::GROUP) {
					JabberSpeak::Instance()->SendMessage(_type, _group_room, message.c_str());
				} else {
					JabberSpeak::Instance()->SendMessage(_type, _user, message.c_str(), _thread);
				}
				
				if (_type == MESSAGE) {
					PostMessage(B_QUIT_REQUESTED);
				} else {
					// client part
					AddToTalk(OurRepresentation().c_str(), message.c_str(), LOCAL);
				}
			} else {
				_message->Insert(message.c_str());
			}
			
			break;
		}

		case JAB_MESSAGE_2: {
			string message = BlabberSettings::Instance()->Data("message-2");

			if (!message.empty() && BlabberSettings::Instance()->Tag("fire-2")) {
				// network part
				if (_type == TalkWindow::GROUP) {
					JabberSpeak::Instance()->SendMessage(_type, _group_room, message.c_str());
				} else {
					JabberSpeak::Instance()->SendMessage(_type, _user, message.c_str(), _thread);
				}
				
				if (_type == MESSAGE) {
					PostMessage(B_QUIT_REQUESTED);
				} else {
					// client part
					AddToTalk(OurRepresentation().c_str(), message.c_str(), LOCAL);
				}
			} else {
				_message->Insert(message.c_str());
			}

			break;
		}

		case JAB_MESSAGE_3: {
			string message = BlabberSettings::Instance()->Data("message-3");

			if (!message.empty() && BlabberSettings::Instance()->Tag("fire-3")) {
				// network part
				if (_type == TalkWindow::GROUP) {
					JabberSpeak::Instance()->SendMessage(_type, _group_room, message.c_str());
				} else {
					JabberSpeak::Instance()->SendMessage(_type, _user, message.c_str(), _thread);
				}
				
				if (_type == MESSAGE) {
					PostMessage(B_QUIT_REQUESTED);
				} else {
					// client part
					AddToTalk(OurRepresentation().c_str(), message.c_str(), LOCAL);
				}
			} else {
				_message->Insert(message.c_str());
			}

			break;
		}

		case JAB_MESSAGE_4: {
			string message = BlabberSettings::Instance()->Data("message-4");

			if (!message.empty() && BlabberSettings::Instance()->Tag("fire-4")) {
				// network part
				if (_type == TalkWindow::GROUP) {
					JabberSpeak::Instance()->SendMessage(_type, _group_room, message.c_str());
				} else {
					JabberSpeak::Instance()->SendMessage(_type, _user, message.c_str(), _thread);
				}
				
				if (_type == MESSAGE) {
					PostMessage(B_QUIT_REQUESTED);
				} else {
					// client part
					AddToTalk(OurRepresentation().c_str(), message.c_str(), LOCAL);
				}
			} else {
				_message->Insert(message.c_str());
			}

			break;
		}

		case JAB_MESSAGE_5: {
			string message = BlabberSettings::Instance()->Data("message-5");

			if (!message.empty() && BlabberSettings::Instance()->Tag("fire-5")) {
				// network part
				if (_type == TalkWindow::GROUP) {
					JabberSpeak::Instance()->SendMessage(_type, _group_room, message.c_str());
				} else {
					JabberSpeak::Instance()->SendMessage(_type, _user, message.c_str(), _thread);
				}
				
				if (_type == MESSAGE) {
					PostMessage(B_QUIT_REQUESTED);
				} else {
					// client part
					AddToTalk(OurRepresentation().c_str(), message.c_str(), LOCAL);
				}
			} else {
				_message->Insert(message.c_str());
			}

			break;
		}

		case JAB_MESSAGE_6: {
			string message = BlabberSettings::Instance()->Data("message-6");

			if (!message.empty() && BlabberSettings::Instance()->Tag("fire-6")) {
				// network part
				if (_type == TalkWindow::GROUP) {
					JabberSpeak::Instance()->SendMessage(_type, _group_room, message.c_str());
				} else {
					JabberSpeak::Instance()->SendMessage(_type, _user, message.c_str(), _thread);
				}
				
				if (_type == MESSAGE) {
					PostMessage(B_QUIT_REQUESTED);
				} else {
					// client part
					AddToTalk(OurRepresentation().c_str(), message.c_str(), LOCAL);
				}
			} else {
				_message->Insert(message.c_str());
			}

			break;
		}

		case JAB_MESSAGE_7: {
			string message = BlabberSettings::Instance()->Data("message-7");

			if (!message.empty() && BlabberSettings::Instance()->Tag("fire-7")) {
				// network part
				if (_type == TalkWindow::GROUP) {
					JabberSpeak::Instance()->SendMessage(_type, _group_room, message.c_str());
				} else {
					JabberSpeak::Instance()->SendMessage(_type, _user, message.c_str(), _thread);
				}
				
				if (_type == MESSAGE) {
					PostMessage(B_QUIT_REQUESTED);
				} else {
					// client part
					AddToTalk(OurRepresentation().c_str(), message.c_str(), LOCAL);
				}
			} else {
				_message->Insert(message.c_str());
			}

			break;
		}

		case JAB_MESSAGE_8: {
			string message = BlabberSettings::Instance()->Data("message-8");

			if (!message.empty() && BlabberSettings::Instance()->Tag("fire-8")) {
				// network part
				if (_type == TalkWindow::GROUP) {
					JabberSpeak::Instance()->SendMessage(_type, _group_room, message.c_str());
				} else {
					JabberSpeak::Instance()->SendMessage(_type, _user, message.c_str(), _thread);
				}
				
				if (_type == MESSAGE) {
					PostMessage(B_QUIT_REQUESTED);
				} else {
					// client part
					AddToTalk(OurRepresentation().c_str(), message.c_str(), LOCAL);
				}
			} else {
				_message->Insert(message.c_str());
			}

			break;
		}

		case JAB_MESSAGE_9: {
			string message = BlabberSettings::Instance()->Data("message-9");

			if (!message.empty() && BlabberSettings::Instance()->Tag("fire-9")) {
				// network part
				if (_type == TalkWindow::GROUP) {
					JabberSpeak::Instance()->SendMessage(_type, _group_room, message.c_str());
				} else {
					JabberSpeak::Instance()->SendMessage(_type, _user, message.c_str(), _thread);
				}
				
				if (_type == MESSAGE) {
					PostMessage(B_QUIT_REQUESTED);
				} else {
					// client part
					AddToTalk(OurRepresentation().c_str(), message.c_str(), LOCAL);
				}
			} else {
				_message->Insert(message.c_str());
			}

			break;
		}

		case JAB_CHAT_SENT: {
			string message = _message->Text();
			
			// eliminate empty messages
			if (message.empty()) {
				break;
			}

			if (!CommandMessage::IsCommand(message) || CommandMessage::IsLegalCommand(message)) {
				// network part
				if (_type == TalkWindow::GROUP) {
					JabberSpeak::Instance()->SendMessage(_type, _group_room, _message->Text());
				} else {
					JabberSpeak::Instance()->SendMessage(_type, _user, _message->Text(), _thread);
				}
			}
						
			if (_type == MESSAGE) {
				PostMessage(B_QUIT_REQUESTED);
				break;
			}
			
			// user part
			AddToTalk(OurRepresentation().c_str(), message, LOCAL);

			// GUI
			_message->SetText("");
			_message->MakeFocus(true);

			break;
		}
	
		case JAB_CLOSE_CHAT: {
			PostMessage(B_QUIT_REQUESTED);
			break;
		}

		case JAB_RIV: {
			string jabber_org = "http://www.osdrawer.net"; 
			
			char *argv[] = {const_cast<char *>(jabber_org.c_str()), NULL};
			if (!be_roster->IsRunning("text/html"))
				be_roster->Launch("text/html", 1, argv);
			else {
				BMessenger messenger("text/html");
				BMessage msg(B_NETPOSITIVE_OPEN_URL);
				msg.AddString("be:url", jabber_org.c_str());
				messenger.SendMessage(&msg);
			}
	
			break;
		}
				
		case JAB_JABBER_ORG: {
			string jabber_org = "http://www.jabber.org"; 
			
			char *argv[] = {const_cast<char *>(jabber_org.c_str()), NULL};
			if (!be_roster->IsRunning("text/html"))
				be_roster->Launch("text/html", 1, argv);
			else {
				BMessenger messenger("text/html");
				BMessage msg(B_NETPOSITIVE_OPEN_URL);
				msg.AddString("be:url", jabber_org.c_str());
				messenger.SendMessage(&msg);
			}
	
			break;
		}

		case JAB_JABBER_CENTRAL_ORG: {
			string jabber_org = "http://www.jabbercentral.org"; 
			
			char *argv[] = {const_cast<char *>(jabber_org.c_str()), NULL};
			if (!be_roster->IsRunning("text/html"))
				be_roster->Launch("text/html", 1, argv);
			else {
				BMessenger messenger("text/html");
				BMessage msg(B_NETPOSITIVE_OPEN_URL);
				msg.AddString("be:url", jabber_org.c_str());
				messenger.SendMessage(&msg);
			}
	
			break;
		}

		case JAB_JABBER_VIEW_COM: {
			string jabber_org = "http://www.jabberview.com"; 
			
			char *argv[] = {const_cast<char *>(jabber_org.c_str()), NULL};
			if (!be_roster->IsRunning("text/html"))
				be_roster->Launch("text/html", 1, argv);
			else {
				BMessenger messenger("text/html");
				BMessage msg(B_NETPOSITIVE_OPEN_URL);
				msg.AddString("be:url", jabber_org.c_str());
				messenger.SendMessage(&msg);
			}
	
			break;
		}

		case JAB_BE_USERS: {
			string jabber_org = "http://home.t-online.de/home/sascha.offe/jabber.html"; 
			
			char *argv[] = {const_cast<char *>(jabber_org.c_str()), NULL};
			if (!be_roster->IsRunning("text/html"))
				be_roster->Launch("text/html", 1, argv);
			else {
				BMessenger messenger("text/html");
				BMessage msg(B_NETPOSITIVE_OPEN_URL);
				msg.AddString("be:url", jabber_org.c_str());
				messenger.SendMessage(&msg);
			}
	
			break;
		}

		case JAB_FAQ: {
			string user_guide = "http://www.users.uswest.net/~jblanco/jabber-faq.html";
			
			char *argv[] = {const_cast<char *>(user_guide.c_str()), NULL};
			if (!be_roster->IsRunning("text/html"))
				be_roster->Launch("text/html", 1, argv);
			else {
				BMessenger messenger("text/html");
				BMessage msg(B_NETPOSITIVE_OPEN_URL);
				msg.AddString("be:url", user_guide.c_str());
				messenger.SendMessage(&msg);
			}
	
			break;
		}

		case JAB_USER_GUIDE: {
			string user_guide = AppLocation::Instance()->AbsolutePath("resources/user-guide/user_guide.html");
			
			char *argv[] = {const_cast<char *>(user_guide.c_str()), NULL};
			if (!be_roster->IsRunning("text/html"))
				be_roster->Launch("text/html", 1, argv);
			else {
				BMessenger messenger("text/html");
				BMessage msg(B_NETPOSITIVE_OPEN_URL);
				msg.AddString("be:url", user_guide.c_str());
				messenger.SendMessage(&msg);
			}
	
			break;
		}

		case JAB_ROTATE_CHAT_FORWARD: {
			TalkManager::Instance()->RotateToNextWindow(this, TalkManager::ROTATE_FORWARD);
			break;
		}

		case JAB_ROTATE_CHAT_BACKWARD: {
			TalkManager::Instance()->RotateToNextWindow(this, TalkManager::ROTATE_BACKWARD);
			break;
		}

        case JAB_FOCUS_BUDDY: {
	         BlabberMainWindow::Instance()->Activate();
             break;
        }
	}
}

bool TalkWindow::QuitRequested() {
	if (_type == GROUP) {
		JabberSpeak::Instance()->SendGroupUnvitation(_group_room, _group_username);	
	}

	TalkManager::Instance()->RemoveWindow(_thread);
	
	return true;
}

string TalkWindow::OurRepresentation() {
	// use friendly name if you have it
	string user = JabberSpeak::Instance()->CurrentRealName();
			
	// and if not :)
	if (user.empty()) {
		user = UserID(JabberSpeak::Instance()->CurrentLogin()).JabberUsername();
	}

	return user;
}

void TalkWindow::AddToTalk(string username, string message, user_type type) {
	// transform local identity
	if (_type == TalkWindow::GROUP && type == LOCAL) {
		username = _group_username;
	}

	// history
	if (type == LOCAL) {
		// reset chat history index
		_chat_index = -1;
		
		// add latest
		_chat_history.push_front(message);
		
		// prune end
		if (_chat_history.size() > 50) {
			_chat_history.pop_back();
		}
	}

	// no duplicates
	if (_type == TalkWindow::GROUP && type == MAIN_RECIPIENT && username == _group_username) {
		return;
	}
	
	// ignore empty messages
	if (message.empty()) {
		return;
	}

	// prune trailing whitespace
	while (!message.empty() && isspace(message[message.size() - 1])) {
		message.erase(message.size() - 1);
	}

	// create the thin (plain) and thick (bold) font
	BFont thin(be_plain_font);
	BFont thick(be_bold_font);

	thin.SetSize(11.0);
	thick.SetSize(11.0);
			
	// some colors to play with
	rgb_color blue   = {0, 0, 255, 255};
	rgb_color red    = {255, 0, 0, 255};
	rgb_color black  = {0, 0, 0, 255};
	
	// some runs to play with
	text_run tr_thick_blue  = {0, thick, blue};
	text_run tr_thick_red   = {0, thick, red};
	text_run tr_thick_black = {0, thick, black};
	text_run tr_thin_black  = {0, thin, black};

	// some run array to play with (simple)
	text_run_array tra_thick_blue  = {1, {tr_thick_blue}}; 
	text_run_array tra_thick_red   = {1, {tr_thick_red}}; 
	text_run_array tra_thick_black = {1, {tr_thick_black}}; 
	text_run_array tra_thin_black  = {1, {tr_thin_black}}; 

	// add to end of conversation
	bool                         is_command = CommandMessage::IsCommand(message);
 	CommandMessage::command_type comm_type  = CommandMessage::ConvertCommandToMessage(message, username);

	// construct timestamp
	char timestamp[64];
	time_t now = time(NULL);
	struct tm *time_struct = localtime(&now);
	
	strftime(timestamp, 63, "[%R:%S] ", time_struct);

	string time_stamp = timestamp;
	
	if (comm_type == CommandMessage::NORMAL) {
		if (type == MAIN_RECIPIENT) {
			if (_chat->TextLength() > 0) {
				if (_type != GROUP || !BlabberSettings::Instance()->Tag("exclude-groupchat-sounds")) {
					SoundSystem::Instance()->PlayMessageSound();
				}
			}
			
			_chat->Insert(_chat->TextLength(), message.c_str(), message.size(), &tra_thick_blue);
		} else if (type == LOCAL || (_type == GROUP && GetGroupUsername() == username)) {
			_chat->Insert(_chat->TextLength(), message.c_str(), message.size(), &tra_thick_red);
		} else {
			_chat->Insert(_chat->TextLength(), message.c_str(), message.size(), &tra_thick_black);
		}

		// log?
		Log(message.c_str());
	} else if (comm_type == CommandMessage::BAD_SYNTAX) {
		if (type == LOCAL || (_type == GROUP && GetGroupUsername() == username)) {
			// this command was illegal and now it belongs to the system
			type = OTHER;

			// print usage (most likely)
			_chat->Insert(_chat->TextLength(), message.c_str(), message.size(), &tra_thick_black);
	
			// log?
			Log(message.c_str());
		}
	} else if (is_command && comm_type == CommandMessage::NOT_A_COMMAND) {
		if (type == LOCAL || (_type == GROUP && GetGroupUsername() == username)) {
			message = "You specified an illegal command.\n";

			// this command was illegal and now it belongs to the system
			type = OTHER;

			// print usage (most likely)
			_chat->Insert(_chat->TextLength(), message.c_str(), message.size(), &tra_thick_black);

			// log?
			Log(message.c_str());
		}
	} else {
		if (type == MAIN_RECIPIENT) {
			if (comm_type != CommandMessage::NORMAL_ALERT && SoundSystem::Instance()->AlertSound() != "<none>" && _chat->TextLength() > 0) {
				if (_type != GROUP || !BlabberSettings::Instance()->Tag("exclude-groupchat-sounds")) {
					SoundSystem::Instance()->PlayMessageSound();
				}
			}

			if (BlabberSettings::Instance()->Tag("show-timestamp")) {
				_chat->Insert(_chat->TextLength(), time_stamp.c_str(), time_stamp.size(), &tra_thin_black);

				// log?
				Log(time_stamp.c_str());
			}
			
			_chat->Insert(_chat->TextLength(), username.c_str(), username.size(), &tra_thick_blue);

			// log?
			Log(username.c_str());

			_chat->Insert(_chat->TextLength(), ": ", 2, &tra_thin_black);

			// log?
			Log(": ");

			text_run_array *this_array;
			GenerateHyperlinkText(message, tr_thin_black, &this_array);
			_chat->Insert(_chat->TextLength(), message.c_str(), message.size(), this_array);

			free(this_array);

			// log?
			Log(message.c_str());

			_chat->Insert(_chat->TextLength(), "\n", 1, &tra_thin_black);

			// log?
			Log("\n");
		} else if (type == LOCAL || (_type == GROUP && GetGroupUsername() == username)) {
			if (BlabberSettings::Instance()->Tag("show-timestamp")) {
				_chat->Insert(_chat->TextLength(), time_stamp.c_str(), time_stamp.size(), &tra_thin_black);

				// log?
				Log(time_stamp.c_str());
			}

			_chat->Insert(_chat->TextLength(), username.c_str(), username.size(), &tra_thick_red);

			// log?
			Log(username.c_str());

			_chat->Insert(_chat->TextLength(), ": ", 2, &tra_thin_black);

			// log?
			Log(": ");

			text_run_array *this_array;
			GenerateHyperlinkText(message, tr_thin_black, &this_array);
			_chat->Insert(_chat->TextLength(), message.c_str(), message.size(), this_array);

			free(this_array);

			// log?
			Log(message.c_str());

			_chat->Insert(_chat->TextLength(), "\n", 1, &tra_thin_black);

			// log?
			Log("\n");
		} else {
			// system message
			_chat->Insert(_chat->TextLength(), message.c_str(), message.size(), &tra_thick_black);

			// log?
			Log(message.c_str());

			_chat->Insert(_chat->TextLength(), "\n", 1, &tra_thick_black);

			// log?
			Log("\n");
		}

		if (comm_type == CommandMessage::NORMAL_ALERT) {
			if (!BlabberSettings::Instance()->Tag("suppress-alert")) {
				Activate();

				// play sound
				if (type != LOCAL) {
					SoundSystem::Instance()->PlayAlertSound();
				}
			}
		}
	}
			
	_chat->ScrollTo(0.0, _chat->Bounds().bottom);
}

void TalkWindow::Log(const char *buffer) {
	if (_am_logging) {
		fwrite(buffer, sizeof(char), strlen(buffer), _log);
		fflush(_log);
	}
}
	
void TalkWindow::NewMessage(string new_message) {
	if (_type == TalkWindow::GROUP) {
		return; // GCHAT
	} else {
		if (!_user->FriendlyName().empty()) {
			AddToTalk(_user->FriendlyName().c_str(), new_message, MAIN_RECIPIENT);
		} else {
			AddToTalk(_user->JabberUsername().c_str(), new_message, MAIN_RECIPIENT);
		}
	}
}

void TalkWindow::NewMessage(string username, string new_message) {
	AddToTalk(username.c_str(), new_message, MAIN_RECIPIENT);
}
	 
void TalkWindow::SetThreadID(string id) {
	_thread = id;
}

const UserID *TalkWindow::GetUserID() {
	return _user;
}

string TalkWindow::GetGroupRoom() {
	return _group_room;
}

string TalkWindow::GetGroupUsername() {
	return _group_username;
}

bool TalkWindow::NewlinesAllowed() {
	return _command_enter->Value();
}

int TalkWindow::CountHyperlinks(string message) {
	string::size_type curr_pos = 0, link_start, link_end;
	string::size_type find1, find2, find3;
	
	// keep count
	int link_count = 0;
	
	// find next link
	link_start = message.find("http://", curr_pos);

	find1 = message.find("ftp://", curr_pos);
	if (find1 != string::npos && (link_start == string::npos || find1 < link_start)) {
		link_start = find1;
	}

	find2 = message.find("www.", curr_pos);
	if (find2 != string::npos && (link_start == string::npos || find2 < link_start)) {
		// ignore if it's not at the beginning or has no whitespace
		if ((find2 - 1) >= 0 && isalnum(message[find2 - 1])) {
			// do nothing
 		} else if (isspace(message[find2 + 4]) || message[find2 + 4] == '.') {
			// do nothing
		} else {
			link_start = find2;
		}
	}

	find3 = message.find("ftp.", curr_pos);
	if (find3 != string::npos && (link_start == string::npos || find3 < link_start)) {
		// ignore if it's not at the beginning or has no whitespace
		if ((find3 - 1) >= 0 && isalnum(message[find3 - 1])) {
			// do nothing
 		} else if (isspace(message[find3 + 4]) || message[find3 + 4] == '.') {
			// do nothing
		} else {
			link_start = find3;
		}
	}

	while (link_start != string::npos) {
		// find whitespace or end
		link_end = message.find_first_of(" \t\r\n", link_start);

		if (link_end == string::npos) {
			link_end = message.size() - 1;
		}

		// prune punctuation
		while (link_start < link_end) {
			if (message[link_end] == ',' || message[link_end] == '!' || message[link_end] == '.' || message[link_end] == ')' || message[link_end] == ';' || message[link_end] == ']' || message[link_end] == '>' || message[link_end] == '\'' || message[link_end] == '"') {
				--link_end;
			} else {
				break;
			}
		}
		
		if (link_start < link_end) {
			++link_count;
		}
		
		curr_pos = link_end + 1;
		
		// find next link
		link_start = message.find("http://", curr_pos);

		find1 = message.find("ftp://", curr_pos);
		if (find1 != string::npos && (link_start == string::npos || find1 < link_start)) {
			link_start = find1;
		}

		find2 = message.find("www.", curr_pos);
		if (find2 != string::npos && (link_start == string::npos || find2 < link_start)) {
			// ignore if it's not at the beginning or has no whitespace
			if ((find2 - 1) >= 0 && isalnum(message[find2 - 1])) {
				// do nothing
	 		} else if (isspace(message[find2 + 4]) || message[find2 + 4] == '.') {
				// do nothing
			} else {
				link_start = find2;
			}
		}
	
		find3 = message.find("ftp.", curr_pos);
		if (find3 != string::npos && (link_start == string::npos || find3 < link_start)) {
			// ignore if it's not at the beginning or has no whitespace
			if ((find3 - 1) >= 0 && isalnum(message[find3 - 1])) {
				// do nothing
	 		} else if (isspace(message[find3 + 4]) || message[find3 + 4] == '.') {
				// do nothing
			} else {
				link_start = find3;
			}
		}
	}

	return link_count;
}

void TalkWindow::GenerateHyperlinkText(string message, text_run standard, text_run_array **tra) {
	int link_count = CountHyperlinks(message);
	string::size_type find1, find2, find3;
	int link_index = 0;

	// no links?
	if (link_count == 0) {
		*tra = (text_run_array *)malloc(sizeof(text_run_array));

		(*tra)->count = 1;
		(*tra)->runs[0].offset = standard.offset;
		(*tra)->runs[0].font = standard.font;
		(*tra)->runs[0].color = standard.color;

		return;
	}
		
	*tra = (text_run_array *)malloc(sizeof(text_run_array) + (sizeof(text_run) * (link_count * 2 - 1)));
	(*tra)->count = link_count * 2;
		
	string::size_type curr_pos = 0, link_start, link_end;

	// find next link
	link_start = message.find("http://", curr_pos);

	find1 = message.find("ftp://", curr_pos);
	if (find1 != string::npos && (link_start == string::npos || find1 < link_start)) {
		link_start = find1;
	}

	find2 = message.find("www.", curr_pos);
	if (find2 != string::npos && (link_start == string::npos || find2 < link_start)) {
		// ignore if it's not at the beginning or has no whitespace
		if ((find2 - 1) >= 0 && isalnum(message[find2 - 1])) {
			// do nothing
 		} else if (isspace(message[find2 + 4]) || message[find2 + 4] == '.') {
			// do nothing
		} else {
			link_start = find2;
		}
	}

	find3 = message.find("ftp.", curr_pos);
	if (find3 != string::npos && (link_start == string::npos || find3 < link_start)) {
		// ignore if it's not at the beginning or has no whitespace
		if ((find3 - 1) >= 0 && isalnum(message[find3 - 1])) {
			// do nothing
 		} else if (isspace(message[find3 + 4]) || message[find3 + 4] == '.') {
			// do nothing
		} else {
			link_start = find3;
		}
	}
			
	while (link_start != string::npos) {
		// find whitespace or end
		link_end = message.find_first_of(" \t\r\n", link_start);

		if (link_end == string::npos) {
			link_end = message.size() - 1;
		}

		// prune punctuation
		while (link_start < link_end) {
			if (message[link_end] == ',' || message[link_end] == '!' || message[link_end] == '.' || message[link_end] == ')' || message[link_end] == ';' || message[link_end] == ']' || message[link_end] == '>' || message[link_end] == '?' || message[link_end] == '\'' || message[link_end] == '"') {
				--link_end;
			} else {
				break;
			}
		}
		
		// add hyperlink
		if (link_start < link_end) {
			BFont thin(be_plain_font);
			rgb_color purple = {192, 0, 192, 255};
			
			(*tra)->runs[link_index].offset = link_start;
			(*tra)->runs[link_index].font = thin;
			(*tra)->runs[link_index].color = purple;

			(*tra)->runs[link_index + 1].offset = link_end + 1;
			(*tra)->runs[link_index + 1].font = standard.font;
			(*tra)->runs[link_index + 1].color = standard.color;
		}
		
		curr_pos = link_end + 1;

		if (curr_pos >= message.size()) {
			break;
		}
		
		// find next link
		link_start = message.find("http://", curr_pos);

		find1 = message.find("ftp://", curr_pos);
		if (find1 != string::npos && (link_start == string::npos || find1 < link_start)) {
			link_start = find1;
		}

		find2 = message.find("www.", curr_pos);
		if (find2 != string::npos && (link_start == string::npos || find2 < link_start)) {
			// ignore if it's not at the beginning or has no whitespace
			if ((find2 - 1) >= 0 && isalnum(message[find2 - 1])) {
				// do nothing
	 		} else if (isspace(message[find2 + 4]) || message[find2 + 4] == '.') {
				// do nothing
			} else {
				link_start = find2;
			}
		}

		find3 = message.find("ftp.", curr_pos);
		if (find3 != string::npos && (link_start == string::npos || find3 < link_start)) {
			// ignore if it's not at the beginning or has no whitespace
			if ((find3 - 1) >= 0 && isalnum(message[find3 - 1])) {
				// do nothing
	 		} else if (isspace(message[find3 + 4]) || message[find3 + 4] == '.') {
				// do nothing
			} else {
				link_start = find3;
			}
		}

		link_index += 2;
	}
}

void TalkWindow::AddGroupChatter(string user) {
	int i;

	// create a new entry
	PeopleListItem *people_item = new PeopleListItem(_group_username, user);
	
	// exception
	if (_people->CountItems() == 0) {
		// add the new user
		_people->AddItem(people_item);

		return;
	}

	// add it to the list
	for (i=0; i < _people->CountItems(); ++i) {
		PeopleListItem *iterating_item = dynamic_cast<PeopleListItem *>(_people->ItemAt(i));

		if (strcasecmp(iterating_item->User().c_str(), user.c_str()) > 0) {
			// add the new user
			_people->AddItem(people_item, i);
			break;
		} else if (!strcasecmp(iterating_item->User().c_str(), user.c_str()) && strcmp(iterating_item->User().c_str(), user.c_str()) > 0) {
			// add the new user
			_people->AddItem(people_item, i);
			break;
		} else if (!strcasecmp(iterating_item->User().c_str(), user.c_str()) && !strcmp(iterating_item->User().c_str(), user.c_str())) {
			_people->InvalidateItem(i);
			break;
		} else if (!strcasecmp(iterating_item->User().c_str(), user.c_str()) && strcmp(iterating_item->User().c_str(), user.c_str()) < 0) {
			if (i == (_people->CountItems() - 1)) {
				// add the new user
				_people->AddItem(people_item);
			} else {
				PeopleListItem *next_item = dynamic_cast<PeopleListItem *>(_people->ItemAt(i + 1));
				
				if (!next_item) {
					// add the new user
					_people->AddItem(people_item);

					break;					
				}

				if (strcasecmp(user.c_str(), next_item->User().c_str()) < 0) {
					// add the new user
					_people->AddItem(people_item, i + 1);
				} else if (!strcasecmp(user.c_str(), next_item->User().c_str())) {
					continue;
				}
			}

			break;
		} else if ((strcasecmp(iterating_item->User().c_str(), user.c_str()) < 0) && (i == (_people->CountItems() - 1))) {
			// add the new user
			_people->AddItem(people_item);

			break;
		} else if (strcasecmp(iterating_item->User().c_str(), user.c_str()) < 0) {
			continue;
		}
	}
}

void TalkWindow::RemoveGroupChatter(string username) {
	// remove user
	for (int i=0; i < _people->CountItems(); ++i) {
		if (dynamic_cast<PeopleListItem *>(_people->ItemAt(i))->User() == username) {
			_people->RemoveItem(i);
		}
	}
}

void TalkWindow::RevealPreviousHistory() {
	// boundary
	if (_chat_index == 49 || _chat_index == (_chat_history.size() - 1)) {
		return;
	}

	if (_chat_index == -1) {
		_chat_buffer = _message->Text();
	}

	// go back
	++_chat_index;
	
	// update text
	_message->SetText(_chat_history[_chat_index].c_str());
}

void TalkWindow::RevealNextHistory() {
	// boundary
	if (_chat_index == -1) {
		return;
	}

	// go forward
	--_chat_index;

	// last buffer
	if (_chat_index == -1) {
		_message->SetText(_chat_buffer.c_str());
	} else {
		// update text
		_message->SetText(_chat_history[_chat_index].c_str());
	}
}

