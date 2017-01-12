// javascript for managing FullCalendar display and populating the reserve form on calendar time selects

var qtip = false;  // assume we don't have the qtip library to start

(function ($) {

    $(document).ready(function () {

	    // try to prevent double form-submits when user double-clicks
        $("#stanford-r25-reservation").submit(function(e){
            $('.form-submit').attr('disabled','disabled');
        });

        // set the date field in the reserve form as the initial focus for the form
        $('#edit-stanford-r25-booking-date-datepicker-popup-0').focus();

        // if we are coming back from a reservation, check cookies for date to bring the user back to
        var defaultDate = readCookie("stanford-r25-date");
        if (defaultDate === null) {
            // if no cookie, see if a date was set by Drupal from a URL parameter
            if (Drupal.settings.hasOwnProperty("stanfordR25ParamDate")) {
                defaultDate = Drupal.settings.stanfordR25ParamDate;
            } else {
                // otherwise, just use today's date
                defaultDate = new Date();
            }
        }
        // cookie would be a single-use thing, so delete it
        deleteCookie("stanford-r25-date");

        // if we are coming back from  a reservation, check cookies for the calendar view
        var defaultView = readCookie("stanford-r25-view");
        if (defaultView === null) {
            // if no cookie, see if a view was set by Drupal from a URL parameter
            if (Drupal.settings.hasOwnProperty("stanfordR25ParamView")) {
                defaultView = Drupal.settings.stanfordR25ParamView;
            } else {
                // otherwise, use the Default view set by Drupal for this room
                switch (Drupal.settings.stanfordR25DefaultView) {
                    case 1:
                        defaultView = 'agendaDay';
                        break;
                    case 2:
                        defaultView = 'agendaWeek';
                        break;
                    case 3:
                        defaultView = 'month';
                        break;
                    default:
                        // finally, default to month view if no other choice
                        defaultView = 'month';
                }
            }
        }
        // delete single-use cookie from reservation
        deleteCookie("stanford-r25-view");

        // get the romm id set on the server in Drupal
        var stanford_r25_room = Drupal.settings.stanfordR25Room;

        // allow the use of qtip tooltips if available and the user's permissions and the room's settings are appropos.
        if (Drupal.settings.stanfordR25Qtip == 'qtip' && Drupal.settings.stanfordR25Access == 1 && Drupal.settings.stanfordR25Status > 0) {
            qtip = true;
        }
        
        // get the room status to see if it is enabled
        var stanford_r25_status = Drupal.settings.stanfordR25Status;

        // the calendar is selectable by the user if the room is bookable and the user has access
        var multiDay = false;  // typically do not allow multi-day reservation
        var selectConstraint = {start: '06:00', end: '22:00'};  // limit selection to "normal" hours
        var selectable = false;  // value of selectable will determine if user can select timeslots from fullcalendar
        if (parseInt(stanford_r25_status) > 1 && parseInt(Drupal.settings.stanfordR25Access) == 1) {
            // in this case, the room is reservable and the user has access to reserve it
            selectable = true;
            if (parseInt(Drupal.settings.stanfordR25MultiDay) == 1) {
                // for multi-day rooms, remove the hour constraint
                multiDay = true;
                selectConstraint = {};
            }
        }
        
        // some rooms constrain how far into the future a user can reserve. 
        var calendarLimit = new Date(parseInt(Drupal.settings.stanfordR25CalendarLimitYear),
            parseInt(Drupal.settings.stanfordR25CalendarLimitMonth));

        // get the maximum selectable duration of the room
        var maxDuration = 0;
        var dValue = Drupal.settings.stanfordR25MaxHours;
        if (!isNaN(dValue) && parseInt(Number(dValue)) == dValue &&
            !isNaN(parseInt(dValue, 10)) && parseInt(dValue, 10) > -1) {
            maxDuration = parseInt(dValue) * 60;
        } else {
            selectable = false;
        }

        // as mentioned above, when the user submits a reservation requests, save the date and calendar view to cookies
        $('#stanford-r25-reservation').submit(function (event) {
            var view = $('#calendar').fullCalendar('getView');
            document.cookie = "stanford-r25-view=" + view.name;
            document.cookie = "stanford-r25-date=" + $('#edit-stanford-r25-booking-date-datepicker-popup-0').val();
            return true;
        });
        
        // configure the fullcalendar plugin
        $('#calendar').fullCalendar({
            allDaySlot: false,
            // if in month view for a non-multi-day room and the user clicks a date, go to agenda day view
            dayClick: function (date, jsEvent, view) {
                if (view.name === "month" && !multiDay) {
                    $('#calendar').fullCalendar('gotoDate', date);
                    $('#calendar').fullCalendar('changeView', 'agendaDay');
                }
            },
            // set the default date and view, either from our cookies (see above) or for current date and month
            defaultDate: defaultDate,
            defaultView: defaultView,
            // sets a "more events" popup if more than two events in a month view day
            eventLimit: true,
            // if we have qtip access, set the tooltips for events
            eventRender: function (event, element) {
                if (qtip) {
                    element.qtip({
                        content: {
                            text: event.tip,
                        },
                        show: {
                            event: 'click'
                        },
                        hide: {
                            event: 'unfocus'
                        },
                        position: {
                            at: "left-bottom",
                            my: 'left-top',
                        }
                    });
                }
            },
            viewRender: function(view, element) {
                // when rendering the calendar, add a permalink to the date and view if room config added permalink id
                if ($('#permalink').length) {
                    var permalink = location.origin + location.pathname +
                        '?view=' + view.name + '&date=' +
                        $('#calendar').fullCalendar('getDate').format('YYYY-MM-DD');
                    $('#permalink').html('<a href="' + permalink + '">Permalink to this page</a>');
                }
                // if there is an upper limit on calendar view, hide (or show) the 'Next' button
                if ( calendarLimit < view.end) {
                    $("#calendar .fc-next-button").hide();
                    return false;
                }
                else {
                    $("#calendar .fc-next-button").show();
                }
            },
            // define how FullCalendar gets events by calling the r25_feed callback function
            events: {
                url: 'r25_feed',
                type: 'POST',
                data: {
                    room_id: stanford_r25_room,
                },
                error: function () {
                    $('#stanford-r25-self-serve-msg').html('Unable to retrieve room schedule from 25Live.');
                },
            },
            // set up which calendar controls we want to show
            header: {
                left: 'prev,next today',
                center: 'title',
                right: 'month,agendaWeek,agendaDay'
            },
            // display the blinky while loading events from the r25_feed json callback
            loading: function (bool) {
                if (bool) {
                    $('body').css("cursor", "progress");
                } else {
                    $('body').css("cursor", "default");
                }
            },
            // when the user clicks and drags to select a date and time, populate the date, time, and duration fields
            // in the reservation form and set the focus to the required headcount field. Also display an error alert
            // if the user tries to select more than the meximum minutes duration
            select: function (start, end) {
                $('#edit-stanford-r25-booking-date-datepicker-popup-0').val(start.format('YYYY-MM-DD'));
                $('#edit-stanford-r25-booking-date-timeEntry-popup-1').val(start.format('hh:mm a'));
                // account for multi-day rooms that have an end date/time instead of a duration
                if (multiDay) {
                    $('#edit-stanford-r25-booking-enddate-datepicker-popup-0').val(end.format('YYYY-MM-DD'));
                    $('#edit-stanford-r25-booking-enddate-timeEntry-popup-1').val(end.format('hh:mm a'));
                } else {
                    var duration = end.diff(start, 'minutes');
                    if (maxDuration > 0 && duration > maxDuration) {
                        var maxStr = '';
                        if (maxDuration > 120) {
                            maxStr = (maxDuration / 60) + ' hours';
                        } else {
                            maxStr = maxDuration + ' minutes';
                        }
                        alert('Maximum booking duration is ' + maxStr + '. For longer please contact a department administrator.');
                    } else {
                        var duration_index = (duration / 30) - 1;
                        $('#edit-stanford-r25-booking-duration').val(duration_index);
                     }
                }
                $('#edit-stanford-r25-booking-headcount').focus();
            },
            // set whether the calendar is selectable, as defined up above
            selectable: selectable,
            // don't let the user select across multiple days unless multi-day room (selectConstraint object set at top)
            selectConstraint: selectConstraint,
            // don't let users select time slots that cross existing reservations
            selectOverlap: false,
            // set default timezone
            timezone: Drupal.settings.stanfordR25Timezone,
        });
    });

    // read a javascript cookie
    function readCookie(name) {
        var nameEQ = name + "=";
        var ca = document.cookie.split(';');
        for (var i = 0; i < ca.length; i++) {
            var c = ca[i];
            while (c.charAt(0) == ' ') c = c.substring(1, c.length);
            if (c.indexOf(nameEQ) == 0) return c.substring(nameEQ.length, c.length);
        }
        return null;
    }

    // delete a javascript cookie
    function deleteCookie(name) {
        document.cookie = name + '=; expires=Thu, 01 Jan 1970 00:00:01 GMT;';
    }


})(jQuery);

