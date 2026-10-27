# The core: all the logic, no screens and no board. Plain C (cJSON for parsing), so the
# PC renderer and its tests compile exactly these files too.
#   wx.c          the forecast: parsing, parts of the day, moon phase
#   wb_request.c  the URLs and requests the board sends
#   alerts.c      NWS alerts and their kid words
#   cal.c         the Google Calendar reader (repeats, tags, filter words, My Day's list)
#   events.c      special events
#   update.c      the over-the-air update manifest
#   outfit.c      Boris's 2.0 outfit engine;  cat.c  his seasonal touches
#   advice.c      the classic screens' clothing advice
#   countdown.c   the nearest countdown and its words;  dress.c  dress-up days
#   routine.c     the Routine page: lists, the time bar, the words, the day's ticks
#   wb_state.h    everything the screens show, in one struct
idf_component_register(
    SRCS "wx.c" "wb_request.c" "events.c" "cal.c" "alerts.c" "update.c" "outfit.c" "cat.c" "advice.c"
         "countdown.c" "dress.c" "routine.c"
    INCLUDE_DIRS "."
    REQUIRES json)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-format-truncation)
