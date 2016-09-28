// javascript for managing FullCalendar display and populating the reserve form on calendar time selects

var qtip = false;  // assume we don't have the qtip library to start

(function ($) {

    $(document).ready(function () {

        // set the date field in the reserve form as the initial focus for the form
        $('#edit-stanford-r25-booking-date-datepicker-popup-0').focus();

        // if we are coming back from a reservation, check cookies for date and calendar view to bring the user back to
        var defaultDate = readCookie("stanford-r25-date");
        if (defaultDate === null) defaultDate = new Date();
        deleteCookie("stanford-r25-date");
        var defaultView = readCookie("stanford-r25-view");
        if (defaultView === null) {
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
                    defaultView = 'agendaWeek';
            }
            // defaultView = "month";
        }
        deleteCookie("stanford-r25-view");

        // get the romm id set on the server in Drupal
        var stanford_r25_room = Drupal.settings.stanfordR25Room;

        // allow the use of qtip if available and the user's permissions and the room's settings are appropos.
        if (Drupal.settings.stanfordR25Qtip == 'qtip' && Drupal.settings.stanfordR25Access == 1 && Drupal.settings.stanfordR25Status > 1) {
            qtip = true;
        }
        var stanford_r25_status = Drupal.settings.stanfordR25Status;

        // the calendar is selectable by the user if the room is bookable and the user has access
        var selectable = false;
        if (parseInt(stanford_r25_status) > 1 && parseInt(Drupal.settings.stanfordR25Access) == 1) {
            selectable = true;
        }

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

        $('#calendar').fullCalendar({
            allDaySlot: false,
            // if in month view and the user clicks a date, go to agenda day view
            dayClick: function (date, jsEvent, view) {
                if (view.name === "month") {
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
                if ($('#permalink').length) {
                    var permalink = location.origin + location.pathname +
                        '?view=' + view.name + '&date=' +
                        $('#calendar').fullCalendar('getDate').format('YYYY-MM-DD');
                    $('#permalink').html('<a href="' + permalink + '">Permalink to this page</a>');
                    //console.log('permalink: ' + permalink);
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
                    $('#edit-stanford-r25-booking-date-datepicker-popup-0').val(start.format('YYYY-MM-DD'));
                    $('#edit-stanford-r25-booking-date-timeEntry-popup-1').val(start.format('hh:mm a'));
                    $('#edit-stanford-r25-booking-headcount').focus();
                }
            },
            // set whether the calendar is selectable, as defined up above
            selectable: selectable,
            // don't let the user select across multiple days
            selectConstraint: {
                start: '00:01',
                end: '23:59',
            },
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

