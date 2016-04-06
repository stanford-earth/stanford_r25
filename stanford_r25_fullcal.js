var qtip = false;

(function($) {

  $(document).ready(function() {

      var defaultDate = readCookie("stanford-r25-date");
      if (defaultDate === null) defaultDate = new Date();
      deleteCookie("stanford-r25-date");
      var defaultView = readCookie("stanford-r25-view");
      if (defaultView === null) defaultView = "month";
      deleteCookie("stanford-r25-view");

      var stanford_r25_room = Drupal.settings.stanfordR25Room;
      if (Drupal.settings.stanfordR25Qtip == 'qtip' && Drupal.settings.stanfordR25Access == 1 && Drupal.settings.stanfordR25Status > 1) {
          qtip = true;
      }
      var stanford_r25_status = Drupal.settings.stanfordR25Status;
      var selectable = false;
      if (parseInt(stanford_r25_status) > 1 && parseInt(Drupal.settings.stanfordR25Access) == 1) {
          selectable = true;
      }
      /*
      $('#stanford-r25-room-show').click(function() {
          $('#stanford-r25-room-info').show();
          $(this).hide();
      });
      $('#stanford-r25-room-hide').click(function() {
          $('#stanford-r25-room-show').show();
          $('#stanford-r25-room-info').hide();
      });
      */
      $('#-stanford-r25-reservation').submit(function(event) {
          var view = $('#calendar').fullCalendar('getView');
          document.cookie = "stanford-r25-view="+view.name;
          document.cookie = "stanford-r25-date="+  $('#edit-stanford-r25-booking-date-datepicker-popup-0').val();
          return true;
      });

      $('#calendar').fullCalendar({
          dayClick: function(date, jsEvent, view) {
              if (view.name === "month") {
                  $('#calendar').fullCalendar('gotoDate', date);
                  $('#calendar').fullCalendar('changeView', 'agendaDay');
              }
          },
          defaultDate: defaultDate,
          defaultView: defaultView,
          eventLimit: true,
          eventRender: function(event, element) {
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
          events: {
              url: 'r25_feed',
              type: 'POST',
              data: {
                  room_id: stanford_r25_room,
              },
              error: function() {
                  $('#stanford-r25-self-serve-msg').html('Unable to retrieve room schedule from 25Live.');
              },
          },
          header: {
              left: 'prev,next today',
              center: 'title',
              right: 'month,agendaWeek,agendaDay'
          },
          loading: function(bool) {
              if (bool) {
                  $('body').css("cursor", "progress");
              } else {
                  $('body').css("cursor", "default");
                  $('#edit-stanford-r25-booking-date-datepicker-popup-0').focus();
              }
          },
          select: function(start, end) {
              var duration = end.diff(start,'minutes');
              if (duration > 120) {
                  alert('Maximum booking duration is 2 hours. For longer please contact a department administrator.');
              } else {
                  var duration_index = (duration / 30) - 1;
                  $('#edit-stanford-r25-booking-duration').val(duration_index);
                  $('#edit-stanford-r25-booking-date-datepicker-popup-0').val(start.format('YYYY-MM-DD'));
                  $('#edit-stanford-r25-booking-date-timeEntry-popup-1').val(start.format('hh:mm a'));
                  $('#edit-stanford-r25-booking-headcount').focus();
              }
          },
          selectable: selectable,
          selectConstraint:{
              start: '00:01',
              end: '23:59',
          },
          selectOverlap: false,
          timezone: Drupal.settings.stanfordR25Timezone,
      });
  });

    function readCookie(name) {
        var nameEQ = name + "=";
        var ca = document.cookie.split(';');
        for(var i=0;i < ca.length;i++) {
            var c = ca[i];
            while (c.charAt(0)==' ') c = c.substring(1,c.length);
            if (c.indexOf(nameEQ) == 0) return c.substring(nameEQ.length,c.length);
        }
        return null;
    }

    function deleteCookie( name ) {
        document.cookie = name + '=; expires=Thu, 01 Jan 1970 00:00:01 GMT;';
    }


})(jQuery);

