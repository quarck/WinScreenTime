using NLog;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using Telegram;
using TimeControlTgBot.Configuration;

namespace TimeControlTgBot.Telegram
{
    public class TimeControlTelegramBot : TelegramBotCore
    {
        private static NLog.Logger logger = NLog.LogManager.GetCurrentClassLogger();

        private readonly ScreenTimeConfig _screenTimeConfig;
        private readonly ExtraTimeConfig _extraTimeConfig;

        private readonly int screenTimeUserId;

        public TimeControlTelegramBot(
            ITelegramBotApi api, 
            string configurationPath = "C:\\ProgramData\\TPService\\conf",
            int myScreenTimeUserId = 1,
            int numWorkers=10
            ) 
            : base(api, numWorkers)
        {
            screenTimeUserId = myScreenTimeUserId;

            _screenTimeConfig = new ScreenTimeConfig(Path.Combine(configurationPath, "configuration.txt"));
            _extraTimeConfig = new ExtraTimeConfig(Path.Combine(configurationPath, "extra.txt"));
        }

        internal override void HandleUserMessage(ITelegramBotApi api, Update update)
        {
            HandleUserMessageAsync(api, update).Wait();
        }

        private async Task HandleUserMessageAsync(ITelegramBotApi api, Update update)
        {
            User from = update.Message.From;
            Chat chat = update.Message.Chat;
            long userId = update.Message.From.Id;
            string text = update.Message.Text ?? "";

            string[] commandItems = text.Split(' ');

            if (commandItems.Length < 1)
            {
                return;
            }

            if (Auth.ADMIN_IDS.Contains(userId))
            {
                await HandleCommand(api, update, from, chat, userId, commandItems);
            }
#if DEBUG
            else
            {                
                //
                // Use this functionality to collect user ids for admin authorization
                //
                if (commandItems.Length >= 1 && commandItems[0] == "/start")
                {
                    logger.Debug($"User {userId} has sent /start", userId);
                    await api.RespondToUpdate(update, $"Hello {from.FirstName}. I've seen you.");
                }
            }
#endif
        }

        private async Task HandleCommand(
            ITelegramBotApi api,
            Update update, 
            User from, 
            Chat chat, 
            long userId, 
            string[] parsedCommand
            )
        {
            switch (parsedCommand[0])
            {
                case "/help":
                    await HandleHelpCommand(api, update, from, chat, userId, parsedCommand);
                    break;

                case "/show":
                    await HandleShowCommand(api, update, from, chat, userId, parsedCommand);
                    break;

                case "/set":
                    await HandleSetCommand(api, update, from, chat, userId, parsedCommand);
                    break;

                case "/extra":
                    await HandleExtraCommand(api, update, from, chat, userId, parsedCommand);
                    break;

                case "/status":
                    await HandleStatusCommand(api, update, from, chat, userId, parsedCommand);
                    break;

                case "/today":
                    await HandleTodayCommand(api, update, from, chat, userId, parsedCommand);
                    break;

                default:
                    await api.RespondToUpdate(update, $"Hello {from.FirstName}, I cannot understand {parsedCommand[0]}, try asking for /help");
                    break;
            }
        }

        private async Task HandleTodayCommand(ITelegramBotApi api, Update update, User from, Chat chat, long userId, string[] parsedCommand)
        {
            var todayUsage = Configuration.DataFiles.GetDayUsage(DateTimeHelpers.GetDayNumber());
            var currentExtraTime = _extraTimeConfig.GetExtraTime(screenTimeUserId);

            await api.RespondToUpdate(update, $"Todays usage: {todayUsage} minutes (allowed extra time is {currentExtraTime})");
        }

        private async Task HandleStatusCommand(ITelegramBotApi api, Update update, User from, Chat chat, long userId, string[] parsedCommand)
        {
            uint numDays = 10;

            if (parsedCommand.Length >= 2)
            {
                if (!uint.TryParse(parsedCommand[1], out numDays))
                {
                    await api.RespondToUpdate(update, $"Failed to parse {parsedCommand[1]} as the number of days");
                    return;
                }
                if (numDays > 31)
                {
                    await api.RespondToUpdate(update, $"numDays={numDays} is too big. Max is 31");
                    return;
                }
            }

            var todayDayNr = DateTimeHelpers.GetDayNumber();

            var sb = new StringBuilder();

            for (int i = -(int)numDays; i <= 0; i++)
            {
                var dayNr = todayDayNr + i;

                var dayUsage = Configuration.DataFiles.GetDayUsage(dayNr);
                if (dayUsage.HasValue)
                    sb.AppendLine($"*{DateTimeHelpers.DayNumberToDateStr((long)dayNr)}* - *{dayUsage}* minutes used");
                else
                    sb.AppendLine($"*{DateTimeHelpers.DayNumberToDateStr((long)dayNr)}* - no activity");
            }

            await api.RespondToUpdate(update, sb.ToString(), parse_mode: "Markdown");
        }

        private string DayNrToString(int dayNr)
        {
            return dayNr switch
            {
                1 => "Monday",
                2 => "Tuesday",
                3 => "Wednesday",
                4 => "Thursday",
                5 => "Friday",
                6 => "Saturday",
                7 => "Sunday",
                _ => "Invalid day"
            };
        }

        private async Task HandleShowCommand(ITelegramBotApi api, Update update, User from, Chat chat, long userId, string[] parsedCommand)
        {
            var currentAllowedTime = _screenTimeConfig.GetAllowedTime(screenTimeUserId);
            var currentExtraTime = _extraTimeConfig.GetExtraTime(screenTimeUserId);

            var sb = new StringBuilder();

            foreach (var day in Enumerable.Range(1, 7))
            {
                currentAllowedTime.TryGetValue(day, out uint allowedMinutes);

                sb.AppendLine($"*{DayNrToString(day)}* (day {day}) - *{allowedMinutes}* minutes");
            }
            sb.AppendLine();
            sb.AppendLine($"*Extra* time for today: *{currentExtraTime}* minutes");

            await api.RespondToUpdate(update, sb.ToString(), parse_mode: "Markdown");
        }

        private async Task HandleExtraCommand(ITelegramBotApi api, Update update, User from, Chat chat, long userId, string[] parsedCommand)
        {
            if (parsedCommand.Length < 2)
            {
                await api.RespondToUpdate(update, $"{from.FirstName}, Please give this command an argument. Example of use:\n\n/extra 90\n\n - Set an extra time to 90 minutes (for today)");
                return;
            }

            if (!uint.TryParse(parsedCommand[1], out uint extraMinutes) || extraMinutes > 1440)
            {
                await api.RespondToUpdate(update, $"{from.FirstName}, I cannot parse {parsedCommand[1]} as a number of minutes. Must be a number betwen 0 and 1440.");
                return;
            }

            _extraTimeConfig.UpdateExtraTime(screenTimeUserId, extraMinutes);

            await api.RespondToUpdate(update, $"{from.FirstName}, extra time is set to {extraMinutes} for today");
        }

        private async Task HandleSetCommand(ITelegramBotApi api, Update update, User from, Chat chat, long userId, string[] parsedCommand)
        {
            // e.g. /set 1 90

            if (parsedCommand.Length < 3)
            {
                await api.RespondToUpdate(update, $"{from.FirstName}, Please give this command an argument. Example of use:\n\n/set 1 90\n\n - Set 90 minutes for Monday");
                return;
            }

            if (!int.TryParse(parsedCommand[1], out int dayOfWeek) || dayOfWeek < 1 || dayOfWeek > 7)
            {
                await api.RespondToUpdate(update, $"{from.FirstName}, I cannot parse {parsedCommand[1]} as a day of week. Must be a number between 1 and 7");
                return;
            }

            if (!uint.TryParse(parsedCommand[2], out uint allowedMinutes) || allowedMinutes > 1440)
            {
                await api.RespondToUpdate(update, $"{from.FirstName}, I cannot parse {parsedCommand[2]} as a number of allowed minutes. Must be a number betwen 0 and 1440.");
                return;
            }

            _screenTimeConfig.UpdateAllowedTime(screenTimeUserId, dayOfWeek, allowedMinutes);

            await api.RespondToUpdate(update, $"{from.FirstName}, allowed screen time for day {dayOfWeek} is set to {allowedMinutes} minutes.");
        }

        private async Task HandleHelpCommand(
            ITelegramBotApi api,
            Update update,
            User from,
            Chat chat,
            long userId,
            string[] parsedCommand
            )
        {
            var r = await api.RespondToUpdate(update,
                $@"Hello {from.FirstName}, here are the commands:

/help - show this help message 

/show - show the current screen time settings

/set day minutes - set the screen time allowed for a specific day of the week. Day is 1 for Monday, 2 for Tuesday and so on. 7 for Sunday. 

    Example: /set 1 90 - allows 90 minutes on Mondays

/extra minutes - add extra time in minutes that will be added to today's allowed time

/status - show the overal status for the user - time spent in the last 7 days.

/today - brief summary of today's usage
");
        }

    }
}
