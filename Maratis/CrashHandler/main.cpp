#include <FL/Fl.H>
#include "CrashHandler.h"
#include <string>
#include <HTTPConnection.h>
#include <PacketizedTCP.h>
#include <RakSleep.h>
#include <FL/fl_message.H>

#ifndef REPORT_DOMAIN
#define REPORT_DOMAIN 127.0.0.1
#endif

Fl_Text_Buffer buffer;

void send_report(Fl_Button* button, void*)
{
    RakNet::TCPInterface tcp;

    button->deactivate();

    tcp.Start(4000, 1);
    RakNet::HTTPConnection connection;

    connection.Init(&tcp, REPORT_DOMAIN, 80);

    std::string stack = buffer.text();
    connection.Post("/crash.php", stack.c_str(), "text/html");

    while(1)
    {
        RakNet::Packet *packet = tcp.Receive();
        if(packet)
        {
            connection.ProcessTCPPacket(packet);
            tcp.DeallocatePacket(packet);
        }

        connection.Update();

        if (connection.IsBusy() == false)
        {
            RakNet::RakString fileContents = connection.Read();

            if(fileContents.Find("Sent crash report."))
            {
                fl_message("Successfully sent crash report.");
                exit(0);
            }
            else
            {
                fl_message("Could not send crash report!");
                button->activate();
                return;
            }
        }

        // Prevent 100% cpu usage
        RakSleep(30);
    }
}

int main(int argc, char **argv)
{
    Fl::get_system_colors();
    Fl::scheme("gtk+");

    CrashHandlerDlg dlg;
    Fl_Double_Window* window = dlg.create_window();

    dlg.stack_output->buffer(&buffer);

#ifndef _WIN32
    std::string path = getenv("HOME");
    path += "/.neoeditor/logfile.txt";

    buffer.loadfile(path.c_str());
#else
    std::string path = getenv("APPDATA");
    path += "\\neoeditor\\logfile.txt";

    buffer.loadfile(path.c_str());
#endif
    window->show();
    return Fl::run();
}
