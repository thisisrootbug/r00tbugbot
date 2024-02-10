#include <stdio.h>
#include <tgbot/tgbot.h>
#include <iostream>
#include <pqxx/pqxx>
#include <string>
#include <thread>
#include <time.h>
#include <codecvt>
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif
#include <wx/editlbox.h>
 
const std::string GetCurrentDateTime() {
    time_t     now = time(0);
    struct tm  tstruct;
    char       buf[80];
    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "%Y-%m-%d %X", &tstruct);
    return buf;
}

class User;

class MyFrame : public wxFrame
{
public:
    MyFrame();
    wxListBox* UsernamesListBox;
    void ShowMessage(wxString message);
    void SendMessage(wxString message);
    void InitUsernames(); 
    User GetCurrentUser();
 
private:
    wxListBox* MessagesHistoryBox;
    wxArrayString MessageHistoryList;
    wxTextCtrl* MessageTextCtrl;
    void OnExit(wxCommandEvent& event);
    void OnAbout(wxCommandEvent& event);
    void OnClickSend(wxCommandEvent& event);
    void OnMouseEvent(wxMouseEvent& event);
    void OnKeyEvent(wxKeyEvent& event);
};
 
MyFrame *frame;


class User {
public: 
    wxString UserName;
    wxString FirstName;
    wxString LastName;
    int64_t ChatId;
    wxString LastSeen;
    User(wxString username){
        try {
            pqxx::connection cr;
            if (cr.is_open()){
                cr.prepare("get_user","select * from usernames where username ilike $1;");
                pqxx::work work(cr);
                work.exec("CREATE TABLE IF NOT EXISTS usernames(username VARCHAR PRIMARY KEY , firstname VarChar NOT NULL,LastName VARCHAR , ChatId BIGINT, LastSeen VARCHAR);");
                work.commit();
                pqxx::result result = work.exec_prepared("get_user", username.ToStdString());
                if (!result.empty()) {
                    for (auto const &row: result) {
                        this->UserName = row[0].c_str();
                        this->FirstName = row[1].c_str();
                        this->LastName = row[2].c_str();
                        this->ChatId = row[3].as<int64_t>();
                        this->LastSeen = row[4].c_str();
                        return;
                    }
                }
            }else{
                std::cout << "Can't connect to database" << std::endl;
            }
        } catch (const std::exception &e) { std::cerr << e.what() << std::endl; }
    }
    User(TgBot::Message::Ptr message){
        try {
            pqxx::connection cr;
            if (cr.is_open()){
                cr.prepare("add_user","insert into usernames values($1, $2 , $3 , $4 , $5 );");
                cr.prepare("get_user","select * from usernames where username ilike $1;");
                cr.prepare("update_chatid","update usernames set chatid=$1 where username = $2;");
                pqxx::work work(cr);
                work.exec("CREATE TABLE IF NOT EXISTS usernames(username VARCHAR PRIMARY KEY , firstname VarChar NOT NULL,LastName VARCHAR , ChatId BIGINT, LastSeen VARCHAR);");
                work.commit();
                pqxx::result result = work.exec_prepared("get_user",message->from->username.c_str());
                if (!result.empty()) {
                    for (auto const &row: result) {
                        this->UserName = row[0].c_str();
                        this->FirstName = row[1].c_str();
                        this->LastName = row[2].c_str();
                        if(message->chat->id !=  row[3].as<int64_t>()){
                            this->ChatId = message->chat->id; 
                            work.exec_prepared("update_chatid", message->chat->id, message->from->username.c_str());
                        }else{
                            this->ChatId = row[3].as<int64_t>();
                        }
                        this->LastSeen = row[4].c_str();
                        return;
                    }
                }else{
                    this->UserName = message->from->username;
                    this->FirstName = message->from->firstName;
                    this->LastName = message->from->lastName;
                    this->ChatId = message->chat->id;
                    this->LastSeen = GetCurrentDateTime();
                    work.exec_prepared("add_user",
                           this->UserName.ToStdString(),
                           this->FirstName.ToStdString(),
                           this->LastName.ToStdString(),
                           this->ChatId,
                           this->LastSeen.ToStdString()
                   );
                }
            }else{
                std::cout << "Can't connect to database" << std::endl;
        }
		} catch (const std::exception &e) {
				std::cerr << e.what() << std::endl;
		}

    }
    void AddToList(){
        if(frame->UsernamesListBox->FindString(this->UserName) == wxNOT_FOUND){
            wxArrayString UsernamesList;
            UsernamesList.Add(this->UserName);
            frame->UsernamesListBox->InsertItems(UsernamesList,frame->UsernamesListBox->GetCount());
        }
    }
};


std::string DEFAULT_REPLY = "What should i say ?";
bool insert_to_db = false;
std::string last_message = "";

std::string get_reply(std::string message) {
		try {
        pqxx::connection cr;
        if (cr.is_open()){
            cr.prepare("get_reply","select reply from replies where message ilike $1;");
            cr.prepare("set_reply","insert into replies(message, reply) values($1, $2);");
            pqxx::work work(cr);
            work.exec("CREATE TABLE IF NOT EXISTS replies(ID SERIAL PRIMARY KEY , message TEXT NOT NULL,reply TEXT NOT NULL);");
            work.commit();
            if(insert_to_db){
                pqxx::result result = work.exec_prepared("set_reply",last_message,message);
                insert_to_db = false;
                return "OK"; 
            }
            pqxx::result result = work.exec_prepared("get_reply",message);
            if (!result.empty()) {
                for (auto const &row: result) {
                    insert_to_db = false;
                    return row[0].c_str();
                }
            }
        }else{
            std::cout << "Can't connect to database" << std::endl;
        }
		} catch (const std::exception &e) {
				std::cerr << e.what() << std::endl;
		}
    insert_to_db = true;
    return DEFAULT_REPLY; 
}


class R00tBugBot : public wxApp
{
public:
    virtual bool OnInit();
};
 

wxIMPLEMENT_APP(R00tBugBot); // this is main loop

bool R00tBugBot::OnInit()
{
    frame = new MyFrame();
    frame->SetClientSize(1920,935);
    frame->Center();
    frame->Show(true);
    return true;
}
 

MyFrame::MyFrame() : wxFrame(NULL, wxID_ANY, "r00tbug bot")
{
    wxMenu *menuFile = new wxMenu;
    menuFile->AppendSeparator();
    menuFile->Append(wxID_EXIT);
    wxMenu *menuHelp = new wxMenu;
    menuHelp->Append(wxID_ABOUT);
    wxMenuBar *menuBar = new wxMenuBar;
    menuBar->Append(menuFile, "&File");
    menuBar->Append(menuHelp, "&Help");
    SetMenuBar( menuBar );
 
    CreateStatusBar();
 
    Bind(wxEVT_MENU, &MyFrame::OnAbout, this, wxID_ABOUT);
    Bind(wxEVT_MENU, &MyFrame::OnExit, this, wxID_EXIT);

    wxPanel* UsernamesPanel = new wxPanel(this);
    wxPanel* MessagesPanel = new wxPanel(this);
    wxPanel* MessagesHistoryPanel = new wxPanel(MessagesPanel);
    wxPanel* MessagesControlsPanel = new wxPanel(MessagesPanel);

    wxButton* send = new wxButton(MessagesControlsPanel, wxID_ANY, wxString::FromUTF8("Send"));
    wxButton* attach = new wxButton(MessagesControlsPanel, wxID_ANY, wxString::FromUTF8("Attach"));
    MessageTextCtrl = new wxTextCtrl(MessagesControlsPanel, wxID_ANY);

    UsernamesListBox = new wxListBox(UsernamesPanel, wxID_ANY);
    MessagesHistoryBox = new wxListBox(MessagesHistoryPanel, wxID_ANY);

    wxSizer *UsernamesSizer  = new wxBoxSizer(wxHORIZONTAL);
    UsernamesSizer->Add(UsernamesListBox,1,wxEXPAND );
    UsernamesPanel->SetSizerAndFit(UsernamesSizer);

    wxSizer *MessagesHistorySizer  = new wxBoxSizer(wxHORIZONTAL);
    MessagesHistorySizer->Add(MessagesHistoryBox,1,wxEXPAND );
    MessagesHistoryPanel->SetSizerAndFit(MessagesHistorySizer);

    wxSizer *ControlsSizer = new wxBoxSizer(wxHORIZONTAL);
    ControlsSizer->Add(MessageTextCtrl,5,wxEXPAND );
    ControlsSizer->Add(send,1,wxEXPAND );
    ControlsSizer->Add(attach,1,wxEXPAND);

    MessagesControlsPanel->SetSizerAndFit(ControlsSizer);

    wxSizer *MessagesSizer = new wxBoxSizer(wxVERTICAL);
    MessagesSizer->Add(MessagesHistoryPanel,15,wxEXPAND | wxALL );
    MessagesSizer->Add(MessagesControlsPanel,1,wxEXPAND | wxALL );

    MessagesPanel->SetSizerAndFit(MessagesSizer);

    wxBoxSizer *sizer = new wxBoxSizer(wxHORIZONTAL);
    sizer->Add(MessagesPanel, 4, wxEXPAND | wxALL );
    sizer->Add(UsernamesPanel, 1, wxEXPAND | wxALL );

    this->SetSizerAndFit(sizer);

    send->Bind(wxEVT_BUTTON, &MyFrame::OnClickSend, this);
    MessageTextCtrl->Bind(wxEVT_CHAR_HOOK, &MyFrame::OnKeyEvent, this);
    UsernamesListBox->Bind(wxEVT_LEFT_DOWN, &MyFrame::OnMouseEvent, this);

    InitUsernames(); 

    const auto MainBotLoop = [this](){
        auto bot = TgBot::Bot(std::getenv("TGBOT_TOKEN"));
        auto longPoll = TgBot::TgLongPoll(bot);
        bot.getEvents().onAnyMessage([&bot](TgBot::Message::Ptr message) {
                auto user = new User(message);
                user->AddToList();
                frame->ShowMessage(wxString::FromUTF8(message->text));
                auto reply = get_reply(message->text);
                frame->MessageTextCtrl->SetValue(wxString::FromUTF8(reply));
        });
        try {
            while (true) { longPoll.start(); }
        } catch (TgBot::TgException& e) { printf("error: %s\n", e.what()); }
    };

    std::thread BotThread{MainBotLoop};
    BotThread.detach();

    SetStatusText(wxString::FromUTF8("Ready.."));

}
 
void MyFrame::OnExit(wxCommandEvent& event) { Close(true); }
void MyFrame::OnAbout(wxCommandEvent& event) { wxMessageBox("This is a simple telegram bot ui with a history of messages and autoreply from history build with C++ wxWidgets by Hasabalrasool Yaseen (r00tbug) while learning about telegram bots and wxWidgets", "r00tbug bot version 1.0", wxOK | wxICON_INFORMATION); }
void MyFrame::OnClickSend(wxCommandEvent& event) { SendMessage(MessageTextCtrl->GetValue());event.Skip();  }
void MyFrame::OnMouseEvent(wxMouseEvent& event) { 
    SetStatusText(wxString::Format("OnMouseEvent( %d , %d )\n", event.GetX() , event.GetY())); 
    event.Skip();
}
void MyFrame::OnKeyEvent(wxKeyEvent& event) { 
    if(event.GetKeyCode() == 13) // enter key
        SendMessage(MessageTextCtrl->GetValue());
    event.Skip();
}

void MyFrame::ShowMessage(wxString message) { 
    if(message == "" ){ return; }
    MessagesHistoryBox->Insert(message,MessagesHistoryBox->GetCount());
    MessageTextCtrl->Clear();
    MessageTextCtrl->SetFocus();
}
void MyFrame::SendMessage(wxString message) { 
    auto user = GetCurrentUser();
    if(user.UserName == ""){ 
        SetStatusText(wxString::FromUTF8("Username is Empty")); 
        return;
    }
    if(message == ""){ 
        SetStatusText(wxString::FromUTF8("Message is Empty")); 
        return;
    }
    auto bot = TgBot::Bot(std::getenv("TGBOT_TOKEN"));
    try {
        std::wstring wide = message.ToStdWstring();
        std::wstring_convert<std::codecvt_utf8<wchar_t>> WStringToUTF8;
        std::string text = WStringToUTF8.to_bytes(wide);
        bot.getApi().sendMessage(GetCurrentUser().ChatId, text);
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
		}
    ShowMessage(message);
    last_message = message;
}

User MyFrame::GetCurrentUser() { 
    if(UsernamesListBox->GetCount() == 0){ 
        return User("");
    }
    if(UsernamesListBox->GetCount() == 1){ 
        UsernamesListBox->SetSelection(0);
    }
    auto username = UsernamesListBox->GetStringSelection();
    return User(username);
}
void MyFrame::InitUsernames() { 
    wxArrayString usernameslist;
    try {
        pqxx::connection cr;
        if (cr.is_open()){
            cr.prepare("get_users","select username from usernames;");
            pqxx::work work(cr);
            work.exec("CREATE TABLE IF NOT EXISTS usernames(username VARCHAR PRIMARY KEY , firstname VarChar NOT NULL,LastName VARCHAR , ChatId BIGINT, LastSeen VARCHAR);");
            work.commit();
            pqxx::result result = work.exec_prepared("get_users");
            if (!result.empty()) {
                for (auto const &row: result) { usernameslist.Add(row[0].c_str()); }
            }else{ std::cout << "result is empty" << std::endl; }
        }else{
            std::cout << "Can't connect to database" << std::endl;
        }
    } catch (const std::exception &e) {
				std::cerr << e.what() << std::endl;
		}
    UsernamesListBox->Clear();
    if(!usernameslist.IsEmpty()){
        UsernamesListBox->InsertItems(usernameslist,0);
    }
}
