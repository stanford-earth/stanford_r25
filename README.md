# stanford_r25

1. Introduction
This module connects a Drupal site via RESTFUL web API to Collegenet's R25
system, which is the backend of the 25Live Room Reservation system. With it,
one can configure a set of rooms for which you can display room info from
25Live, calendars from either the 25Live Publisher or using the FullCalendar
JavaScript plugin, and use a Drupal form to create non-recurring events in
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
where organization is the name of your school, such as stanford

You will also need to enter the name of a room photos subdirectory to be created
under your public Drupal files directory. The module will store room photos there
pulled from the API.

When you enter your account, password, the URL, and the directory name, the system will
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

On this form you can also set:
* a login message and url for the reservation form if it is restricted to
  the non-anonymous role (recommended)
* a no-permission message to be displayed in lieu of the reservations form to
  non-anonymous users who do not have permission to make reservations
* a read-only message to be displayed in lieu of the reservations form for
  rooms that don't allow reservations
* reservation instructions that should be displayed as part of the reservation
  form. You can use the token [max_duration] to insert the maximum booking duration
  for each room.
* blackout dates listed in the form "YYYY-MM-DD - YYYY-MM-DD" when this module may not make
  reservations for rooms marked as honoring blackouts. Does not use 25Live blackout periods
  because rooms may need to be reservable by other processes such as registrar room assignment.
* link text to be used for externally authenticated (non-Drupal) users, along with a description
  of the hooks needed to do this. See, also, the user0_webauth submodule for more info

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
* Room Display options: "Disabled" => no calendar view is displayed and new
  events can not be created for this room; "Read Only" => calendar displays
  events but new events can not be created from this module; "Tentative
  Bookings" => calendar displays events and new reservations may be created
  but they are marked "Tentative" in 25Live; "Confirmed Bookings" => calendar
  displays events and new events may be created that are automatically
  marked as "Confirmed" in 25Live.
* Default Calendar View: For Fullcalendar, whether the room's calendar
  should initially be displayed in Daily Agenda view, Weekly view, or Monthly
  view.s
* Maximum Reservation Hours: the maximum number of hours, from 1 t 24, a reservation for
  this room can be via this module. Setting to 0 equals 24. Ignored for multi-day rooms.
* 25Live Publisher Webname - this is the calendar "spud" name created by the
  25Live Publisher for a room. Required if calendar type is 25Live Publisher.
* R25 Room ID: room code from 25Live for the room. Required if room type is
  "tentative" or "confirmed" booking. Can be found from the event.xml request
  described above with the tag "r25:space_id".
* Show Event Description instead of Event Name in FullCalendar.
* Display a permalink on FullCalendar pages so user can return to that month, week, or day view.
* Honor blackout dates for reservations for this room if blackout dates were entered on configuration tab.
* Approver Security Group: 25Live Security group name (also may be a Stanford workgroup) of those able
  to confirm tentative reservation requests. They will receive emails notification of tentative reservations.
* Email Cancellations to Approvers: will email members of the Approver Security Group if someone cancels
  a reservation using this module.
* Email List: comma-separated list of additional email addresses that should receive a
  notification of each tentative or confirmed reservation made from this module. Useful for debugging.
* Advanced Options:
  * Authentication method: if external authentication is used, whether the room allows reservations by
    Drupal accounts (with reservation permission) only, external accounts only, or either one.
  * Allow multi-day reservations: Instead of a start day/time and duration, the reservation form will
    include a start day/time and an end day/time. Fullcalendar will allow the user to click and drag
    across days to create a reservation.
  * Event Attributes: If your event type includes custom 25Live attributes, enter their comma-separated
    ids in this field and the attribute fields will be added to the reservation form.
  * Contact Attrbute: If your event type includes a custom 25Live attribute used for collecting contact info,
    include its attribute id here and the field will be added to the reservation form.
  * Auto-Bill Rate Group ID: If you are using the 25Live pricing feature, you can enter the id of a billing rate group
    here and it will be automatically set for reservations for this room.
  * Override booking instructions: if you want different booking instructions for this room than the default set on
    the configuration tab, this field is for you.
  * Postprocess Booking: checkbox will cause room booking information to be placed in the reservation form's
    storage array for use in a custom submit hook.

Upon creation of a room configuration, the maximum headcount for the room is
retrieved from the web services API and stored along with the entered room
information, as well as the room's picture if available, and the email addresses
associated with the room's security group.

If the maximum headcount number for a room or the room's picture is changed in 25Live,
or if there are changes to the members of the room's security group, the room configuration
in the Drupal module should be edited and saved to get the new values.

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
