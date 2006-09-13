// Copyright 2003-6 Max Howell <max.howell@methylblue.com>
// Redistributable under the terms of the GNU General Public License

#include <cstdio>         //popen, fread
#include "crashHandler.h"
#include "define.h"
#include <iostream>
#include <kapplication.h> //invokeMailer()
#include <kdebug.h>       //kdBacktrace()
#include <kdeversion.h>
#include <klocale.h>
#include <ktempfile.h>
#include <qregexp.h>
#include <qglobal.h>      //qVersion()
#include <sys/types.h>    //pid_t
#include <sys/wait.h>     //waitpid
#include <unistd.h>       //write, getpid


static QString
run_command( const QCString &command )
{
    static const uint SIZE = 40960; //40 KiB
    static char buf[ SIZE ] = { 0 };

    std::cout << "Running: " << command << std::endl;

    FILE *process = ::popen( command, "r" );
    if (process) {
        int const n = std::fread( (void*)buf, sizeof(char), SIZE-1, process );
        buf[ n ] = '\0';
        ::pclose( process );
    }

    return QString::fromLocal8Bit( buf );
}

void
mxcl::crashHandler( int /*signal*/ )
{
    // we need to fork to be able to get a semi-decent bt - I dunno why
    pid_t const pid = ::fork();

    if (pid < 0) {
        std::cout << "Filelight has crashed...\n" << "fork() failed\n";
        _exit( 1 );
    }
    else if (pid != 0) {
        // we are the process that crashed
        ::alarm( 0 );
        ::waitpid( pid, NULL, 0 ); // wait for child to exit
        ::_exit( 253 );
    }
    else {
        // we are the child process - the result of the fork()

        std::cout << "Filelight has crashed...\n";

        //TODO -fomit-frame-pointer buggers up the backtrace, so detect it
        //TODO -O optimization can rearrange execution and stuff so show a warning for the developer
        //TODO pass the CXXFLAGS used with the email

        QString subject = APP_VERSION " ";

        /// obtain the backtrace with gdb

        KTempFile temp;
        temp.setAutoDelete( true );

        QCString const gdb_command =
                "bt\n"
                "echo\\n\\n\n"
                "echo ==== (gdb) bt full ================\n"
                "bt full\n"
                "echo\\n\\n\n"
                "echo ==== (gdb) thread apply all bt ====\n"
                "thread apply all bt\n";

        int const handle = temp.handle();

        ::write( handle, gdb_command, gdb_command.length() );
        ::fsync( handle );

        // so we can read stderr too
        ::dup2( fileno( stdout ), fileno( stderr ) );

        QCString gdb;
        gdb  = "gdb --nw -n --batch -x ";
        gdb += temp.name().latin1();
        gdb += " filelight ";
        gdb += QCString().setNum( ::getppid() );

        QString bt = run_command( gdb );

        /// clean up
        bt.remove( "(no debugging symbols found)..." );
        bt.remove( "(no debugging symbols found)\n" );
        //bt.replace( QRegExp("\n{2,}"), "\n" ); //clean up multiple \n characters
        bt.stripWhiteSpace();

        /// analyze usefulness
        bool useful = true;

        if (bt.isEmpty())
            useful = false;
        else {
            const int invalidFrames = bt.contains( QRegExp("\n#[0-9]+\\s+0x[0-9A-Fa-f]+ in \\?\\?") );
            const int validFrames = bt.contains( QRegExp("\n#[0-9]+\\s+0x[0-9A-Fa-f]+ in [^?]") );
            const int totalFrames = invalidFrames + validFrames;

            if (totalFrames > 0) {
                double const validity = double(validFrames) / totalFrames;
                subject += QString("[validity: %1]").arg( validity, 0, 'f', 2 );
                if (validity <= 0.3)
                    useful = false;
            }
            subject += QString("[frames: %1]").arg( totalFrames, 3 /*padding*/ );

            if (bt.find( QRegExp(" at \\w*\\.cpp:\\d+\n") ) >= 0)
                subject += "[line numbers]";
        }

        /// determine if stripped
        const QString file_out = run_command( "file `which filelight`" );

        if (file_out.find( "not stripped", false ) == -1)
            subject += "[___stripped]"; //same length as below
        else
            subject += "[NOTstripped]";

        std::cout << subject.latin1() << std::endl;

        if (useful) {
            QString body = i18n(
                    "Filelight has crashed! :(\n\n"

                    "But, all is not lost! Information about the crash is embedded in this "
                    "mail, so just click send, and we'll do what we can to fix the crash.\n\n"

                    "Many thanks :)\n\n\n\n\n\n\n\n" ) +

                    "------------------------------------------------------------------------\n" + i18n(
                    "The information below is to help the developers identify the problem, "
                    "please do not modify it.\n\n" );



            body += "==== Build Information ===========\n"
                    "Version:    " APP_VERSION "\n"
                    "Build date: " __DATE__ "\n"
                    "CC version: " __VERSION__ "\n" //assuming we're using GCC
                    "KDElibs:    " KDE_VERSION_STRING "\n"
                    "Qt:         " + QString(qVersion()) + '\n' +
                #ifdef NDEBUG
                    "Debug:      false"
                #else
                    "Debug:      true"
                #endif
                    "\n\n\n";

            body += "==== file `which filelight` =======\n";
            body += file_out + "\n\n";
            body += "==== (gdb) bt =====================\n";
            body += bt + "\n\n";
            body += "==== kdBacktrace() ================\n";
            body += kdBacktrace();

            //TODO startup notification
            kapp->invokeMailer(
                    /*to*/          "backtraces@methylblue.com",
                    /*cc*/          QString(),
                    /*bcc*/         QString(),
                    /*subject*/     subject,
                    /*body*/        body,
                    /*messageFile*/ QString(),
                    /*attachURLs*/  QStringList(),
                    /*startup_id*/  "" );
        }

        //_exit() exits immediately, otherwise this
        //function is called repeatedly ad finitum
        ::_exit( 255 );
    }
}
