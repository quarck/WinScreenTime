using NLog;
using TimeControlTgBot.Telegram;
using System;
using System.IO;
using System.Net.Http;
using System.Threading;
using System.Threading.Tasks;
using Telegram;

namespace TimeControlTgBot
{
    class Program
    {
        const string LOCATION = "C:\\ProgramData\\TPService";

        static void SetupLogging(string path)
        {
            var config = new NLog.Config.LoggingConfiguration();

            var logfile = new NLog.Targets.FileTarget("logfile") {
                FileName = path,
                FileNameKind = NLog.Targets.FilePathKind.Absolute, 
                Layout = "${longdate} ${level:uppercase=true} ${logger}: ${message} ${exception:format=tostring}", 
                ArchiveAboveSize = 1024 * 1024 * 8, 
                MaxArchiveFiles = 10
            };

            var logconsole = new NLog.Targets.ConsoleTarget("logconsole")
            {
                Layout = "${longdate} ${level:uppercase=true} ${logger}: ${message} ${exception:format=tostring}"
            };

#if DEBUG
            config.AddRule(LogLevel.Info, LogLevel.Fatal, logconsole);
#endif
            config.AddRule(LogLevel.Debug, LogLevel.Fatal, logfile);

            NLog.LogManager.Configuration = config;
        }

        static void Main()
        {
            SetupLogging(Path.Combine(LOCATION, "tglog.txt"));           

            var bot =
                new TimeControlTelegramBot(
                    new TelegramBotApi(Auth.TG_API_TOKEN, TimeSpan.FromSeconds(1000))
                );

            Task.Factory.StartNew(() => bot.StartAsync().Wait()).Wait();

            // If any of the tasks above finishes - something has gone wrong, so we terminate 
            // on the any of the tasks termination
        }
    }
}
