# stanford_r25

1. Introduction
This module connects a Drupal site via RESTFUL web API to Collegenet's R25
system, which is the backend of the 25Live Room Reservation system. With it,
one can configure a set of rooms for which you can display room info from
25Live, calendars from either the 25Live Publisher or using the FullCalendar
JavaScript plugin, and use a Drupal webform to create non-recurring events in
25Live.

2. Installation:

2.1 Basic installation and dependencies

Install and enable this module as you would any other Drupal module. You must
also have the encrypt and libraries modules installed and configured, encrypt
to protect your 25Live admin credentials and libraries to easily locate
FullCalendar and qTip modules.

2.2 Optional Javascript libraries

The latest version of FullCalendar, found at http://fullcalendar.io/, is not
required but is recommended, especially if you would like users to select
reservation timeslots from a calendar.

The latest version of qTip 2, found at http://qtip2.com/, is not required but
is recommended if you would like to display event information (headcount and
who made the reservation) in a tool tip on a FullCalendar display.

Both can be installed in the libraries directory.

2.3 Permissions

Permissions are created for roles that can administer this module's
configuration, roles that can view calendars, and roles that can
reserve rooms.

3. Configuration:

Configuration forms can be found on three tabs (credentials, configuration,
and list rooms) on your site at /admin/config/system/stanford_r25.

3.1 Credentials tab

You need a username and password for the 25Live admin console. This account
should have the rights to view room information, events lists, create an event,
and delete an event.

You will also need your organization's base URL for accessing the 25Live web
services API, for example
https://webservices.collegenet.com/r25ws/wrd/<organization>/run
where organization is the name of your school.

You will also need the base URL for displaying room images from 25Live. An
example would be
https://25live.collegenet.com/25live/data/<organization>/run
where organization is the name of your school.

When you enter your account, password, and the two URLs, the system will
make a test call to the 25Live Web Services API. You will be notified if the
call is successful or not.

You will also see if the system detected the FullCalendar and qTip libraries.

3.2 Configuration tab

On the configuration tab you will enter a set of organization and event codes
needed by the 25Live API. You can find some of these codes by viewing an event
in 25Live and then calling the following in your web browser:
https://webservices.collegenet.com/r25ws/wrd/<organization>/run/event.xml?event_locator=<code>
where <organization> is your school and <code> is the Reference Code for the
event.

After being prompted for your account name and password (the same credentials
you entered in the Credentials tab) you should see an XML display.

Find the r25:organization_id tag for your organization code.
Find the r25:event_type_id tag for an organization event type code.
Find the r25:parent_id tag for the parent folder where your new events will be
stored.

On this form you can also:
* set a login message and url for the reservation form if it is restricted to
  the non-anonymous role (recommended)
* a no-permission message to be displayed in lieu of the reservations form to
  non-anonymous users who do not have permission to make reservations
* a read-only message to be displayed in lieu of the reservations form for
  rooms that don't allow reservations
* reservation instructions that should be displayed as part of the reservation
  form. You can use the token [max_duration] to insert the maximum booking duration
  for each room.

3.3 List Rooms tab

This tab will display all rooms you have configured for this module along with
edit and delete links for each. A room's display name is also a link to its
calendar page. An "Add Room Configuration" link allows creation of new rooms.

When creating or editing a room, the following information is collected:
* the room's display name
* the room's machine name - this can be edited when the room is first added to
  the system but is readonly after. The machine name is used in the room page
  URL -- /r25/<machine-name>/calendar -- and is passed as a parameter to the
  blocks containing room information and the reservation form.
* calendar type - either an embedded calendar from 25Live Publisher or the
  FullCalendar JavaScript widget. 25Live Publisher calendars can not be used to
  populate the reservation form and are only updated by the R25 system every
  fifteen to twenty minutes. The FullCalendar widget can be used clicked and
  dragged upon to populate the reservation form and is updated immediately
  upon submitting a new reservation.
* max_hours: the maximum number of hours a reservation for this room can be via 
  this module.
* Room Display options: "Disabled" => no calendar view is displayed and new
  events can not be created for this room; "Read Only" => calendar displays
  events but new events can not be created from this module; "Tentative
  Bookings" => calendar displays events and new reservations may be created
  but they are marked "Tentative" in 25Live; "Confirmed Bookings" => calendar
  displays events and new events may be created that are automatically
  marked as "Confirmed" in 25Live.
* 25Live Publisher Webname - this is the calendar "spud" name created by the
  25Live Publisher for a room. Required if calendar type is 25Live Publisher.
* R25 Room ID: room code from 25Live for the room. Required if room type is
  "tentative" or "confirmed" booking. Can be found from the event.xml request
  described above with the tag "r25:space_id"
* Email List: comma-separated list of email addresses that should receive a
  notification of each tentative or confirmed reservation made from this module.

Upon creation of a room configuration, the maximum headcount for the room is
retrieved from the web services API and stored along with the entered room
information. If the maximum headcount number for a room is changed in 25Live,
the room configuration in the Drupal module should be edited and saved to
get the new value.

4. Pages and Blocks

Each room configured and enabled in 3.3 will generate a page on your site at
/r25/<machine-name>/calendar. The calendar for the room will be displayed in
the main content area of the page.

The system will also generate two blocks: "R25 Room Information" and
"R25 Room Reservation Form". Both blocks are initially configured to appear
only on pages matching "r25/*/calendar" but are not initially placed in a
region. If room reservations are enabled, it is recommended that the
reservation block be placed prominently in a sidebar or content region.

Both blocks figure out their room_id from the current url and will
display an error message if placed on other pages.

5. Possible future enhancements:
5.1 Make room configurations a content type or attachable to other entities.
5.2 Cache room event data retrieved from 25Live. Invalidate cache based
    on time and/or new reservation made.
5.3 Edit/Delete events.
5.4 Recurring events.
--end--
