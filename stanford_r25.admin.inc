<?php

// standard drupal-type administration forms
// this one is for password and service provider url information
function _stanford_r25_credentials($form, &$form_state) {

  // start with an instructive string
  $markup_str = 'Enter your credentials for the R25 API.<br /> Don\'t have credentials? File a HelpSU ticket to Administrative Applications/25 Live.';

  // rely on encrypt module's decrypt function to get the 25Live credential from variables table
  $credentials = explode(':', decrypt(variable_get('stanford_r25_credential', '')), 2);
  $clientid = $credentials[0];
  $password = (isset($credentials[1])) ? $credentials[1] : '';

  $base_url = variable_get('stanford_r25_base_url', 'https://webservices.collegenet.com/r25ws/wrd/stanford/run');

  // see if we can authenticate the current credentials and post a message if we can
  if (_stanford_r25_api_call('test', $base_url, $clientid . ':' . $password)) {
    $markup_str .= '<br /><br />Good news! Your credentials are set and valid and your site can currently connect to the R25 API.';
  }
  $form['description'] = array(
    '#markup' => t($markup_str),
  );

  // fields to enter 25Live credentials
  $form['stanford_r25_clientid'] = array(
    '#type' => 'textfield',
    '#title' => t('Username:'),
    '#default_value' => $clientid,
    '#required' => TRUE,
  );
  $form['stanford_r25_password'] = array(
    '#type' => 'password',
    '#title' => t('Password:'),
    '#default_value' => $password,
    '#required' => TRUE,
  );

  // base URL for calls to the 25Live API such as "https://webservices.collegenet.com/r25ws/wrd/stanford/run"
  $form['stanford_r25_base_url'] = array(
    '#type' => 'textfield',
    '#title' => t('Base URL'),
    '#description' => t('Base URL for R25 Webs Services calls.'),
    '#default_value' => $base_url,
    '#required' => TRUE,
  );

  // A directory name to be created under the Drupal files directory for storage of location photos
  $form['stanford_r25_room_image_directory'] = array(
    '#type' => 'textfield',
    '#title' => t('Room Photos'),
    '#description' => t('Directory under Drupal files directory for storage of R25 location photos.'),
    '#default_value' => variable_get('stanford_r25_room_image_directory', 'R25RoomPhotos'),
    '#required' => FALSE,
  );

  // check if system includes fullcalendar
  if (libraries_get_path('fullcalendar') === FALSE) {
    $fullcalendar_str = 'You do not have the FullCalendar library installed, so you will not be able to set up self-service bookings. You can download the library from http://fullcalendar.io.';
  }
  else {
    $fullcalendar_str = 'Woohoo! You have the FullCalendar library available, so you can set rooms up for self-service bookings!';
  }
  $form['fullcal_msg'] = array(
    '#markup' => '<p>' . t($fullcalendar_str) . '</p>',
  );

  // check if system includes qtip jquery library for tooltips
  if (libraries_get_path('jquery.qtip') === FALSE) {
    $qtip_str = 'You do not have the qTip library installed, so we won\'t be able to display extra information about bookings on the FullCalendar display. You can download the library from  http://qtip2.com.';
  }
  else {
    $qtip_str = 'Yay! You have the qTip library installed, so we can display extra information about bookings on the FullCalendar display when events are clicked!';
  }
  $form['qtip_msg'] = array(
    '#markup' => '<p>' . t($qtip_str) . '</p>',
  );

  // submit button
  $form['actions'] = array('#type' => 'actions');
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save settings'),
  );
  return $form;
}

// validate credentials by calling a test xml command in the webservices api
function _stanford_r25_credentials_validate($form, &$form_state) {
  if (empty($form_state['values']['stanford_r25_clientid'])) {
    form_set_error('stanford_r25_clientid', t("Error. Username and/or Password are missing."));
  }
  else {
    if (empty($form_state['values']['stanford_r25_password'])) {
      form_set_error('stanford_r25_password', t("Error. Username and/or Password are missing."));
    }
    else {
      if (empty($form_state['values']['stanford_r25_password'])) {
        form_set_error('stanford_r25_password', t("Error. Username and/or Password are missing."));
      }
      else {
        if (empty($form_state['values']['stanford_r25_base_url'])) {
          form_set_error('stanford_r25_base_url', t("Error. Base URL is missing."));
        }
        else {
          $username = $form_state['values']['stanford_r25_clientid'];
          $password = $form_state['values']['stanford_r25_password'];
          $base_url = $form_state['values']['stanford_r25_base_url'];
          if (!_stanford_r25_api_call('test', $base_url, $username . ':' . $password)) {
            form_set_error('stanford_r25_password', t("Error. Can't connect to R25 API. Please check your username and password."));
          }
        }
      }
    }
  }

  if (!empty($form_state['values']['stanford_r25_room_image_directory'])) {
    $directory = 'public://' . $form_state['values']['stanford_r25_room_image_directory'];
    if (!file_prepare_directory($directory, FILE_CREATE_DIRECTORY)) {
      form_set_error('stanford_r25_room_image_directory', t('Unable to create writable public directory'));
    }
  }
}

// save the credential information, encrypting it first.
function _stanford_r25_credentials_submit($form, &$form_state) {
  $credential = '';
  if (!empty($form_state['values']['stanford_r25_clientid'])) {
    $credential .= $form_state['values']['stanford_r25_clientid'];
  }
  $credential .= ':';
  if (!empty($form_state['values']['stanford_r25_password'])) {
    $credential .= $form_state['values']['stanford_r25_password'];
  }
  $credential = encrypt($credential);  // use the encrypt module for this
  variable_set('stanford_r25_credential', $credential);
  $base_url = '';
  if (isset($form_state['values']['stanford_r25_base_url'])) {
    $base_url = $form_state['values']['stanford_r25_base_url'];
  }
  variable_set('stanford_r25_base_url', $base_url);

  if (isset($form_state['values']['stanford_r25_room_image_directory'])) {
    variable_set('stanford_r25_room_image_directory', $form_state['values']['stanford_r25_room_image_directory']);
  }

  drupal_set_message(t('Credentials saved.'));

  // the credential we are using to access the 25Live api has an id number associated with it
  // get the 25Live account id# for this credential to use if setting todo's.
  $contact_id = FALSE;
  $results = _stanford_r25_api_call('acctinfo');
  if ($results) {
    if (!empty($results['index']['R25:CONTACT_ID'][0])) {
      $contact_id = $results['vals'][$results['index']['R25:CONTACT_ID'][0]]['value'];
    }
  }
  if (!$contact_id) {
    drupal_set_message(t('Unable to retrieve R25 Contact ID# for credential. Tentative Reservations will not create approval to-dos.'));
  }
  variable_set('stanford_r25_credential_contact_id', $contact_id);
}

// another standard drupal administration form for organization codes and error responses, etc.
function _stanford_r25_settings($form, &$form_state) {

  // various codes to be included in reservation requests, can be gotten by calling various API URLs
  // manually from a browser or, in some cases, by looking at entries in 25Live administration console
  $form['codes'] = array(
    '#type' => 'fieldset',
    '#title' => '25Live Codes',
    '#description' => t('Various codes for your department that need to be sent along with room requests. ' .
      'This module currently only reserves rooms with the single org id and event type code specified here.'),
  );
  $form['codes']['stanford_r25_org_id'] = array(
    '#type' => 'textfield',
    '#title' => t('Organization ID'),
    '#default_value' => variable_get('stanford_r25_org_id'),
    '#required' => TRUE,
  );
  $form['codes']['stanford_r25_event_type'] = array(
    '#type' => 'textfield',
    '#title' => t('Event Type Code'),
    '#default_value' => variable_get('stanford_r25_event_type'),
    '#required' => TRUE,
  );
  $form['codes']['stanford_r25_parent_event_id'] = array(
    '#type' => 'textfield',
    '#title' => t('Parent Event ID'),
    '#default_value' => variable_get('stanford_r25_parent_event_id'),
    '#required' => TRUE,
  );

  // general login settings to use when access is granted to Drupal users via roles and permissions
  $form['login'] = array(
    '#type' => 'fieldset',
    '#title' => 'Booking Restrictions based on Drupal roles and permissions',
    '#description' => t('Use the "Book R25 Rooms" permission to restrict room reservations to specific roles. If ' .
      'anonymous users do not permission to book, the login link specified here will appear in place of the ' .
      'booking form.'),
  );
  $form['login']['stanford_r25_login_msg'] = array(
    '#type' => 'textfield',
    '#title' => t('Anonymous User Login Message'),
    '#description' => t('Potentially helpful reminder for anonymous users that may need to log in before being able to reserve rooms.'),
    '#default_value' => variable_get('stanford_r25_login_msg', 'Reserve this room'),
  );
  $form['login']['stanford_r25_login_uri'] = array(
    '#type' => 'textfield',
    '#title' => t('Login URL'),
    '#description' => t('Login URI for the reservation form if user is anonymous. Login message field must be set for this to show. Defaults to /user/login'),
    '#default_value' => variable_get('stanford_r25_login_uri', '/user/login'),
  );
  $form['login']['login_contact_info'] = array(
    '#type' => 'markup',
    '#markup' => 'Contact information for room reservations will consist of the user account name and email ' .
      'address unless it is overridden by an implementation of hook_stanford_r25_contact_alter(&$contact_string). ' .
      'If multiple modules implement the hook, the value will be set by the last module invoked.',
  );

  // advanced settings for external non-Drupal logins; may require 3rd party contrib module for authentication
  // sample module for authenticating anonymous users, user0_webauth, is included in this package
  $form['external'] = array(
    '#type' => 'fieldset',
    '#title' => 'Advanced Restrictions',
    '#description' => t('If you need to allow room reservations by users who authenticate through an external ' .
      'system but who do not get get Drupal accounts (for example, rooms bookable by entire campus versus ' .
      'rooms bookable only within the organization for which there are Drupal accounts) you must implement ' .
      'hooks for hook_stanford_r25_external_link and hook_stanford_r25_external_user. See the included ' .
      'user0_webauth module for an example.'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );
  $form['external']['stanford_r25_ext_login_msg'] = array(
    '#type' => 'textfield',
    '#title' => t('Unauthenticated User Login Message'),
    '#description' => t('Potentially helpful reminder for anonymous users that may need to log in before being able to reserve rooms.'),
    '#default_value' => variable_get('stanford_r25_ext_login_msg', 'Reserve this room'),
  );
  $form['external']['external_contact_info'] = array(
    '#type' => 'markup',
    '#markup' => 'Contact information for room reservations will blank unless ' .
      'it is set by an implementation of hook_stanford_r25_external_user_display(&$acct_array). ' .
      'If multiple modules implement the hook, the value will be set by the first module found.',
  );

  // messages to display if room is not reservable or user has no permission to reserve
  $default_not_permitted = variable_get('stanford_r25_notpermitted_msg',
    array('value' => '', 'format' => filter_default_format()));
  $form['stanford_r25_notpermitted_msg'] = array(
    '#type' => 'text_format',
    '#title' => t('No Permission to Reserve Rooms Message'),
    '#description' => t('Informational message to logged in users without the "Book R25 Rooms" permission.'),
    '#default_value' => $default_not_permitted['value'],
    '#format' => $default_not_permitted['format'],
  );
  $default_readonly_msg = variable_get('stanford_r25_readonly_msg',
    array('value' => '', 'format' => filter_default_format()));
  $form['stanford_r25_readonly_msg'] = array(
    '#type' => 'text_format',
    '#title' => t('Read Only Calendar Message'),
    '#description' => t('A message informing user that a room is not reservable.'),
    '#default_value' => $default_readonly_msg['value'],
    '#format' => $default_readonly_msg['format'],
  );

  // default booking instructions to appear at bottom or reservation form.
  $default_booking_instr = variable_get('stanford_r25_booking_instructions',
    array('value' => '', 'format' => filter_default_format()));
  $form['stanford_r25_booking_instructions'] = array(
    '#type' => 'text_format',
    '#title' => t('Booking Instructions'),
    '#description' => t('Instructions that will appear above room reservation forms.'),
    '#default_value' => $default_booking_instr['value'],
    '#format' => $default_booking_instr['format'],
  );

  // a list of blackout periods during which users will not be able to reserve rooms that
  // are configured to respect blackouts
  $form['stanford_r25_blackout_dates'] = array(
    '#type' => 'textarea',
    '#title' => t('Blackout Dates'),
    '#description' => t('A list of blackout periods in the form "YYYY-MM-DD - YYYY-MM-DD" when this module may ' .
      'not make reservations for rooms marked as honoring blackouts. Does not use 25Live blackout periods ' .
      'because rooms may need to be reservable by other processes such as registrar room assignment.'),
    '#default_value' => variable_get('stanford_r25_blackout_dates', ''),
  );

  // submit button
  $form['actions'] = array('#type' => 'actions');
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save settings'),
  );
  return $form;
}

// validate that numeric inputs are, in fact, numeric
function _stanford_r25_settings_validate($form, &$form_state) {
  // validate numeric entries
  if (empty($form_state['values']['stanford_r25_org_id']) || !is_numeric($form_state['values']['stanford_r25_org_id'])) {
    form_set_error('stanford_r25_org_id', t('Organization ID must be a number.'));
  }
  if (empty($form_state['values']['stanford_r25_event_type']) || !is_numeric($form_state['values']['stanford_r25_event_type'])) {
    form_set_error('stanford_r25_event_type', t('Event Type Code must be a number.'));
  }
  if (empty($form_state['values']['stanford_r25_parent_event_id']) || !is_numeric($form_state['values']['stanford_r25_parent_event_id'])) {
    form_set_error('stanford_r25_parent_event_id', t('Parent Event ID must be a number.'));
  }

  // validate blackout date list
  if (!empty($form_state['values']['stanford_r25_blackout_dates'])) {
    $blackouts = _stanford_r25_parse_blackout_dates($form_state['values']['stanford_r25_blackout_dates']);
    if (empty($blackouts)) {
      form_set_error('stanford_r25_blackout_dates', t('Blackout dates must be in the form "YYYY-MM-DD - YYYY-MM-DD".'));
    }
  }
}

// save the form's inputs to the variables table
function _stanford_r25_settings_submit($form, &$form_state) {
  // submit function - save all configuration variables from this form to the variables table
  if (isset($form_state['values']['stanford_r25_org_id'])) {
    variable_set('stanford_r25_org_id', $form_state['values']['stanford_r25_org_id']);
  }
  if (isset($form_state['values']['stanford_r25_event_type'])) {
    variable_set('stanford_r25_event_type', $form_state['values']['stanford_r25_event_type']);
  }
  if (isset($form_state['values']['stanford_r25_parent_event_id'])) {
    variable_set('stanford_r25_parent_event_id', $form_state['values']['stanford_r25_parent_event_id']);
  }
  if (isset($form_state['values']['stanford_r25_login_msg'])) {
    variable_set('stanford_r25_login_msg', $form_state['values']['stanford_r25_login_msg']);
  }
  if (isset($form_state['values']['stanford_r25_login_uri'])) {
    variable_set('stanford_r25_login_uri', $form_state['values']['stanford_r25_login_uri']);
  }
  if (isset($form_state['values']['stanford_r25_ext_login_msg'])) {
    variable_set('stanford_r25_ext_login_msg', $form_state['values']['stanford_r25_ext_login_msg']);
  }
  if (isset($form_state['values']['stanford_r25_notpermitted_msg'])) {
    variable_set('stanford_r25_notpermitted_msg',
      array(
        'value' => $form_state['values']['stanford_r25_notpermitted_msg']['value'],
        'format' => $form_state['values']['stanford_r25_notpermitted_msg']['format']
      ));
  }
  if (isset($form_state['values']['stanford_r25_readonly_msg'])) {
    variable_set('stanford_r25_readonly_msg',
      array(
        'value' => $form_state['values']['stanford_r25_readonly_msg']['value'],
        'format' => $form_state['values']['stanford_r25_readonly_msg']['format']
      ));
  }
  if (isset($form_state['values']['stanford_r25_booking_instructions'])) {
    variable_set('stanford_r25_booking_instructions',
      array(
        'value' => $form_state['values']['stanford_r25_booking_instructions']['value'],
        'format' => $form_state['values']['stanford_r25_booking_instructions']['format']
      ));
  }
  if (isset($form_state['values']['stanford_r25_blackout_dates'])) {
    variable_set('stanford_r25_blackout_dates', $form_state['values']['stanford_r25_blackout_dates']);
  }
  drupal_set_message(t('Configuration saved.'));
}

// format a table of the rooms that we've configured, with edit and delete buttons
// based on code found in the encrypt module
function _stanford_r25_room_list() {
  $rooms = _stanford_r25_room_config_load();
  $header = array(
    t('Display Name'),
    t('Calendar Type'),
    t('25Live Publisher Webname'),
    t('25Live Space ID'),
    t('Max Reservation Hrs'),
    t('Created'),
    t('Status'),
    t('Approver Security Group'),
    array('data' => t('Operations'), 'colspan' => '3'),
  );
  $rows = array();

  foreach ($rooms as $key => $config) {
    $row = array();
    $row[] = array('data' => l($config['display_name'], '/r25/' . $key . '/calendar'));
    switch (intval($config['caltype'])) {
      case 1:
        $caltype = t('25Live Publisher');
        break;
      case 2:
        $caltype = t('FullCalendar');
        break;
      default:
        $caltype = t('Unknown');
        break;
    }
    $row[] = array('data' => $caltype);
    $row[] = array('data' => $config['spud_name']);
    $row[] = array('data' => $config['space_id']);
    $row[] = array('data' => $config['max_hours']);
    $row[] = array('data' => $config['created']);
    switch ($config['status']) {
      case 1:
        $status = t('Read-Only Display');
        break;
      case 2:
        $status = t('Tentative Bookings');
        break;
      case 3:
        $status = t('Confirmed Bookings');
        break;
      default:
        $status = t('Disabled');
    }
    $row[] = array('data' => $status);
    $row[] = array('data' => $config['approver_secgroup_name']);
    // Set the edit column.
    $row[] = array('data' => l(t('edit'), STANFORD_R25_MENU_PATH . '/' . $key . '/edit'));

    // Set the delete column.
    $row[] = array('data' => l(t('delete'), STANFORD_R25_MENU_PATH . '/' . $key . '/delete'));

    $rows[] = $row;
  }
  $build['stanford_r25_rooms_list_table'] = array(
    '#theme' => 'table',
    '#header' => $header,
    '#rows' => $rows,
    '#empty' => t('No room configurations are available. <a href="@link">Add a configuration</a>.', array('@link' => url(STANFORD_R25_MENU_PATH . '/list/add'))),
  );
  return $build;
}

// room configuration form for both adding and editing a room
function _stanford_r25_room_config($form, &$form_state, $roomid = NULL) {
  $form = array();
  $config = array();
  if (isset($roomid) && $roomid !== 'add') {
    $rooms = _stanford_r25_room_config_load();
    if (isset($rooms[$roomid])) {
      $config = $rooms[$roomid];
    }
  }
  if (empty($config)) {
    // set default values when configuring a new room
    $config = array(
      'machine_name' => '',
      'display_name' => '',
      'caltype' => 2,
      'spud_name' => '',
      'space_id' => '',
      'email_list' => '',
      'created' => format_date(time()),
      'status' => 1,
      'max_hours' => 2,
      'default_view' => 2,
      'description_as_title' => 0,
      'permalink' => 0,
      'honor_blackouts' => 0,
      'approver_secgroup_name' => '',
      'approver_secgroup_id' => 0,
      'authentication' => 1,
      'multi_day' => 0,
      'event_attributes' => '',
      'contact_attr' => '',
      'postprocess_booking' => 0,
      'auto_billing_code' => '',
      'email_confirms_and_cancels' => 0,
      'override_booking_instructions' => array(
        'value' => '',
        'format' => filter_default_format(),
      ),
      'override_blackout_instructions' => array(
        'value' => '',
        'format' => filter_default_format(),
      ),
    );
  }

  // display name for the room, such as Mitchell 138. It should, but does not have to,
  // match the name of the room in 25Live.
  $form['display_name'] = array(
    '#title' => t('Display Name'),
    '#type' => 'textfield',
    '#default_value' => $config['display_name'],
    '#description' => t('The human-readable name of the configuration.'),
    '#required' => TRUE,
    '#size' => 30,
  );
  // a unique machine name for the room; one will be constructed automatically
  // but better to shorten it to 4 or 5 characters. Will be used in URL to
  // calendar page. For example, use m138 for Mitchell 138 and calendar page
  // will be r25/m138/calendar
  $form['machine_name'] = array(
    '#type' => 'machine_name',
    '#default_value' => $config['machine_name'],
    '#maxlength' => 32,
    '#disabled' => !empty($config['machine_name']),
    '#machine_name' => array(
      'exists' => '_stanford_r25_room_config_exists',
      'source' => array('display_name'),
    ),
    '#description' => t('A unique machine-readable name for the configuration. It must only contain lowercase letters, numbers, and underscores.'),
  );
  // whether the calendar page uses 25Live Publisher embeds or fullcalendar
  // Only fullcalendar allows selection of dates, times, and durations from the calendar
  $form['caltype'] = array(
    '#type' => 'radios',
    '#title' => t('Calendar Display Options'),
    '#default_value' => $config['caltype'],
    '#options' => array(1 => '25Live Publisher', 2 => 'FullCalendar'),
    '#description' => t('Whether to use the 25Live Publisher read-only calendar display or the interactive FullCalendar display.'),
  );

  // room status - whether it should display a calendar and allow reservations, display a calendar without allowing reservations,
  // or display a calendar and allow tentative or confirmed reservations
  $form['status'] = array(
    '#type' => 'radios',
    '#title' => t('Room Display Options'),
    '#default_value' => $config['status'],
    '#options' => array(
      0 => 'Disabled',
      1 => 'Read-Only Calendar',
      2 => 'Tentative Bookings',
      3 => 'Confirmed Bookings'
    ),
    '#description' => t('Whether to just display a calendar or allow tentative or confirmed bookings.'),
  );

  // whether the initial calendar display is Month, Week, or Day
  $form['default_view'] = array(
    '#type' => 'radios',
    '#title' => t('Default Calendar View'),
    '#default_value' => $config['default_view'],
    '#options' => array(1 => 'Daily', 2 => 'Weekly', 3 => 'Monthly'),
    '#description' => t('Whether the initial view of a calendar page should be a monthly, weekly, or daily view. Applies only to FullCalendar.'),
  );

  // Maximum number of hours for a booking. Ignored for multi-day bookable rooms
  $form['max_hours'] = array(
    '#title' => t('Maximum Reservation (Hours)'),
    '#type' => 'textfield',
    '#required' => TRUE,
    '#size' => 30,
    '#default_value' => $config['max_hours'],
    '#description' => t('The maximum number of hours for a reservation via this interface. Set to 0 for no limit.'),
  );

  // 25Live Publisher name of calendar if using it; otherwise leave blank
  $form['spud_name'] = array(
    '#title' => t('25Live Publisher Webname'),
    '#type' => 'textfield',
    //'#reqired' => true,
    '#size' => 30,
    '#default_value' => $config['spud_name'],
    '#description' => t('The 25Live Publisher webname or "spud" name for this room\'s public calendar display. Required for 25Live Publisher display.'),
    '#states' => array(
      'required' => array(
        ':input[name="caltype"]' => array('value' => 1)
      )
    ),
  );

  // Internal 25Live "space id" for the room. Can be found in results from call to https://webservices.collegenet.com/r25ws/wrd/stanford/run/spaces.xml
  // (replacing stanford with your organization name)
  $form['space_id'] = array(
    '#title' => t('R25 Room ID'),
    '#type' => 'textfield',
    '#reqired' => TRUE,
    '#size' => 30,
    '#default_value' => $config['space_id'],
    '#description' => t('The R25 space_id code for this room. Required for tentative and confirmed bookings.'),
    '#states' => array(
      'required' => array(
        ':input[name="status"]' => array(
          array('value' => 2),
          array('value' => 3)
        )
      )
    ),
  );
  // the event description field found in the 25Live event wizard allows more characters than the event title
  $form['description_as_title'] = array(
    '#type' => 'checkbox',
    '#title' => t('Show event description as event name in FullCalendar'),
    '#return_value' => 1,
    '#default_value' => $config['description_as_title'],
    '#description' => t("Check if you would like to use the Event Description field instead of the Event Name in the FullCalendar time slot."),
  );
  // give users a way to get back to a particular fullcalendar date and view
  $form['permalink'] = array(
    '#type' => 'checkbox',
    '#title' => t('Show a permlink on FullCalendar pages for the view and date'),
    '#return_value' => 1,
    '#default_value' => $config['permalink'],
    '#description' => t('Useful if you want to distribute links to specific calendar pages to people.'),
  );
  // have room reservations obey the blackout periods specified for the entire site
  $form['honor_blackouts'] = array(
    '#type' => 'checkbox',
    '#title' => t('Honor Blackout Dates for Reservations'),
    '#return_value' => 1,
    '#default_value' => $config['honor_blackouts'],
    '#description' => t('Only allow reservations if current and requested dates are after the end of the most ' .
      'recent blackout period and before the start of the next blackout period.'),
  );
  // the name of the 25Live security group (ususally the same as a Stanford workgroup, but not necessarily so)
  // this will be used to generate and cache a list of email addresses to be contacted for tentative reservations
  $form['approver_secgroup_name'] = array(
    '#title' => t('Approver Security Group'),
    '#type' => 'textfield',
    '#size' => 30,
    '#default_value' => $config['approver_secgroup_name'],
    '#description' => t('The R25 Security Group, also a Stanford Workgroup, of those who can approve tentative ' .
      'reservation requests. All members of this group will receive email with the request information. '),
  );
  // the 25Live security group id that corresponds to the security group name. looked up on form submit.
  $form['approver_secgroup_id'] = array(
    '#title' => t('Approver Security Group ID'),
    '#type' => 'hidden',
    '#size' => 30,
    '#default_value' => $config['approver_secgroup_id'],
    '#description' => t('The corresponding 25Live id number for the security group specified above.'),
  );
  // whether to email members of the security group and additional email list when an event is canceled or confirmed
  // through this website
  $form['email_confirms_and_cancels'] = array(
    '#type' => 'checkbox',
    '#title' => t('Email cancellations to approvers'),
    '#return_value' => 1,
    '#default_value' => $config['email_confirms_and_cancels'],
    '#description' => t('Check if room approvers should receive an email when a user self-service cancels a reservation.'),
  );
  // additional email addresses to get confirm,cancel, and tentative reservation emails.
  $form['email_list'] = array(
    '#title' => t('Email List'),
    '#type' => 'textfield',
    '#size' => 30,
    '#default_value' => $config['email_list'],
    '#description' => t('Comma-separated list of email addresses which should receive notification of any reservation requests. Leave blank for "none".'),
  );

  // a fieldset of rarely-needed, advanced settings
  $form['advanced'] = array(
    '#type' => 'fieldset',
    '#title' => t('Advanced Options'),
    '#description' => t('Some uncommonly used options.'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );
  // whether the room allows reservations only by drupal permission, or only by external authentication, or either
  $form['advanced']['authentication'] = array(
    '#type' => 'radios',
    '#title' => t('Authentication Method'),
    '#default_value' => $config['authentication'],
    '#options' => array(
      1 => 'Internal (Drupal) Accounts',
      2 => 'External (Non-Drupal) Login',
      3 => 'Both Internal and External'
    ),
    '#description' => t('Whether the room is bookable based on Drupal accounts, roles, and permissions or through external means.'),
  );
  // whether the room may be reserved for multiple-day events
  $form['advanced']['multi_day'] = array(
    '#type' => 'checkbox',
    '#title' => t('Allow multi-day reservations'),
    '#description' => t('Start time will refer to first day; end-time will refer to last day; days in-between will be full days.'),
    '#default_value' => $config['multi_day'],
  );
  // whether there are additional 25Live Event Custom Attribute fields that should be added to the reservation form.
  // provide the comma-separated ids for the wanted attributes; see attribute list at
  // https://webservices.collegenet.com/r25ws/wrd/stanford/run/evatrb.xml (substitute your inst for 'stanford'
  $form['advanced']['event_attributes'] = array(
    '#type' => 'textfield',
    '#title' => t('Event Attributes'),
    '#default_value' => $config['event_attributes'],
    '#description' => t('If custom attribute fields need to be included on the reservation form, enter their R25 ' .
      'id codes as a comma-separated list. Put an asterisk after a number to indicate a required field.'),
  );
  // whether there is an additional contact attribute defined for your event; provide the attribute id as above
  $form['advanced']['contact_attr'] = array(
    '#type' => 'textfield',
    '#title' => t('Contact Attribute'),
    '#default_value' => $config['contact_attr'],
    '#description' => t("Event Custom Attribute in which we will store the user's contact information."),
  );
  // a billing code to use if you want to auto-select a billing code for this event
  $form['advanced']['auto_billing_code'] = array(
    '#type' => 'textfield',
    '#title' => t('Auto-Bill Rate Group ID'),
    '#default_value' => $config['auto_billing_code'],
    '#description' => t("The rate group id to use to auto-bill for the use of this room. Leave blank for none."),
  );
  // override default booking instructions for this room
  $default_booking_instr = $config['override_booking_instructions'];
  $form['advanced']['override_booking_instructions'] = array(
    '#type' => 'text_format',
    '#title' => t('Override Booking Instructions'),
    '#description' => t('Instructions that will appear below room reservation forms if different from site default.'),
    '#default_value' => $default_booking_instr['value'],
    '#format' => $default_booking_instr['format'],
  );
  // checkbox if you want to store booking information in $form_state['storage'] for post-processing in your own module
  $form['advanced']['postprocess_booking'] = array(
    '#type' => 'checkbox',
    '#title' => t('Postprocess Booking'),
    '#description' => t("If you want to write you own submit hook to do something after a booking is complete, check this box and booking info will be placed in \$form_state['storage']"),
    '#default_value' => $config['postprocess_booking'],
  );
  // override default booking instructions for this room
  $default_blackout_instr = $config['override_blackout_instructions'];
  $form['advanced']['override_blackout_instructions'] = array(
    '#type' => 'text_format',
    '#title' => t('Override Blackout Instructions'),
    '#description' => t('User instructions for booking during blackout periods, if "Honor Blackout Dates" is checked. Default site message displayed if left blank.'),
    '#default_value' => $default_blackout_instr['value'],
    '#format' => $default_blackout_instr['format'],
  );
  // set a created date
  $form['created'] = array(
    '#title' => t('Created on'),
    '#type' => 'textfield',
    '#default_value' => $config['created'],
    '#disabled' => TRUE,
  );
  // submit button, but of course
  $form['actions'] = array('#type' => 'actions');
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save configuration'),
    '#submit' => array('_stanford_r25_room_config_submit'),
    '#weight' => 40,
  );
  return $form;
}

// validate the form; make sure responses are consistent with the room status
function _stanford_r25_room_config_validate($form, &$form_state) {
  $status = 0;
  if (isset($form_state['values']['status'])) {
    $status = intval($form_state['values']['status']);
  }
  $caltype = 0;
  if (isset($form_state['values']['caltype'])) {
    $caltype = intval($form_state['values']['caltype']);
  }
  if (empty($form_state['values']['spud_name']) && $caltype == 1 && $status > 0) {
    form_set_error('spud_name', '25Live Publisher webname is required to display calendar.');
  }
  if (empty($form_state['values']['space_id']) && $status > 1) {
    form_set_error('space_id', 'R25 Room ID is required to enable bookings.');
  }
  $max_hours = -1;
  if (isset($form_state['values']['max_hours'])) {
    $max_hours = $form_state['values']['max_hours'];
  }
  if (!is_numeric($max_hours) || intval($max_hours) != $max_hours || $max_hours < 0) {
    form_set_error('max_hours', 'Maximum Reservation (Hours) must be zero or a positive integer.');
  }
  if (empty($form_state['values']['approver_secgroup_name'])) {
    $form_state['values']['approver_secgroup_id'] = 0;
  }
  else {
    $form_state['values']['approver_secgroup_id'] = _stanford_r25_secgroup_id($form_state['values']['approver_secgroup_name']);
    if (empty($form_state['values']['approver_secgroup_id'])) {
      form_set_error('approver_secgroup_name', 'Unable to retrieve security group id from 25Live.');
    }
    else {
      $list = _stanford_r25_security_group_emails($form_state['values']['approver_secgroup_id'], TRUE);
      if (empty($list)) {
        form_set_error('approver_secgroup_name', 'Unable to retrieve security group email list from 25Live.');
      }
    }
  }
}

// save the room configuation; currently saves to a room array stored in the variables table
function _stanford_r25_room_config_submit($form, &$form_state) {
  $fields = array(
    'machine_name' => (string) $form_state['values']['machine_name'],
    'display_name' => (string) $form_state['values']['display_name'],
    'spud_name' => (string) $form_state['values']['spud_name'],
    'space_id' => (string) $form_state['values']['space_id'],
    'email_list' => (string) $form_state['values']['email_list'],
    'created' => (string) $form_state['values']['created'],
    'status' => (int) $form_state['values']['status'],
    'caltype' => (int) $form_state['values']['caltype'],
    'max_hours' => (int) $form_state['values']['max_hours'],
    'location_info' => _stanford_r25_update_room_info($form_state['values']['space_id']),
    'default_view' => (int) $form_state['values']['default_view'],
    'description_as_title' => (int) $form_state['values']['description_as_title'],
    'permalink' => (int) $form_state['values']['permalink'],
    'honor_blackouts' => (int) $form_state['values']['honor_blackouts'],
    'override_blackout_instructions' => array(
      'value' => $form_state['values']['override_blackout_instructions']['value'],
      'format' => $form_state['values']['override_blackout_instructions']['format'],
    ),
    'approver_secgroup_name' => (string) $form_state['values']['approver_secgroup_name'],
    'approver_secgroup_id' => (int) $form_state['values']['approver_secgroup_id'],
    'email_confirms_and_cancels' => (int) $form_state['values']['email_confirms_and_cancels'],
    'authentication' => (int) $form_state['values']['authentication'],
    'multi_day' => (int) $form_state['values']['multi_day'],
    'postprocess_booking' => (int) $form_state['values']['postprocess_booking'],
    'override_booking_instructions' => array(
      'value' => $form_state['values']['override_booking_instructions']['value'],
      'format' => $form_state['values']['override_booking_instructions']['format']
    ),
    'event_attributes' => (string) $form_state['values']['event_attributes'],
    'event_attribute_fields' => _stanford_r25_update_event_attribute_fields($form_state['values']['event_attributes']),
    'contact_attr' => (string) $form_state['values']['contact_attr'],
    'contact_attr_field' => _stanford_r25_update_event_attribute_fields($form_state['values']['contact_attr'], TRUE),
    'auto_billing_code' => (string) $form_state['values']['auto_billing_code'],
  );
  _stanford_r25_room_config_save($fields);
  $form_state['redirect'] = STANFORD_R25_MENU_PATH . '/list';
}

// on submission, query the 25Live database for a bunch of information about the room
// and store in the array with the room confir
function _stanford_r25_update_room_info($space_id = NULL) {
  $room_info = array();
  $default_layout = 0;
  if (!empty($space_id)) {
    // get the room info using the api
    $results = _stanford_r25_api_call('roominfo', NULL, NULL, $space_id);
    if ($results) {
      // identify the default room layout, if one is defined, otherwise use the first one
      if (!empty($results['index']['R25:DEFAULT_LAYOUT']) && is_array($results['index']['R25:DEFAULT_LAYOUT'])) {
        foreach ($results['index']['R25:DEFAULT_LAYOUT'] as $dlkey => $dlval) {
          if ($results['vals'][$dlval]['value'] == 'T') {
            $default_layout = $dlkey;
            break;
          }
        }
      }
      // get the room capacity for the layout
      $room_info['capacity'] = empty($results['index']['R25:LAYOUT_CAPACITY'][$default_layout]) ? NULL :
        $results['vals'][$results['index']['R25:LAYOUT_CAPACITY'][$default_layout]]['value'];
      // get any comments and instructions about the room
      $room_info['comments'] = empty($results['index']['R25:COMMENTS'][0]) ? NULL : $results['vals'][$results['index']['R25:COMMENTS'][0]]['value'];
      $room_info['layout_name'] = empty($results['index']['R25:LAYOUT_NAME'][$default_layout]) ? NULL :
        $results['vals'][$results['index']['R25:LAYOUT_NAME'][$default_layout]]['value'];
      $room_info['layout_instruction'] = empty($results['index']['R25:LAYOUT_INSTRUCTION'][$default_layout]) ? NULL :
        empty($results['vals'][$results['index']['R25:LAYOUT_INSTRUCTION'][$default_layout]]['value']) ? NULL :
          $results['vals'][$results['index']['R25:LAYOUT_INSTRUCTION'][$default_layout]]['value'];
      // build a list of features for the room layout
      $layout_features = '';
      $first_feature = TRUE;
      if (!empty($results['index']['R25:FEATURE_NAME'][0])) {
        foreach ($results['index']['R25:FEATURE_NAME'] as $index) {
          if (!$first_feature) {
            $layout_features .= ', ';
          }
          $layout_features .= $results['vals'][$index]['value'];
          $first_feature = FALSE;
        }
      }
      $room_info['layout_features'] = empty($layout_features) ? NULL : $layout_features;
      // build a list of categories for the room
      $layout_categories = '';
      $first_category = TRUE;
      if (!empty($results['index']['R25:CATEGORY_NAME'][0])) {
        foreach ($results['index']['R25:CATEGORY_NAME'] as $index) {
          if (!$first_category) {
            $layout_categories .= ', ';
          }
          $layout_categories .= $results['vals'][$index]['value'];
          $first_category = FALSE;
        }
      }
      $room_info['layout_categories'] = empty($layout_categories) ? NULL : $layout_categories;

      // get a room photo if available and store in Drupal files
      $photo_id = NULL;
      $image_directory = variable_get('stanford_r25_room_image_directory', NULL);
      if (!empty($image_directory) &&
        !empty($results['index']['R25:LAYOUT_PHOTO_ID'][$default_layout])
      ) {
        $photo_id = $results['vals'][$results['index']['R25:LAYOUT_PHOTO_ID'][$default_layout]]['value'];
        $photo = _stanford_r25_api_call('roomphoto', NULL, NULL, $photo_id);
        if ($photo) {
          $destination = _stanford_r25_file_path($photo_id);
          if (!file_save_data($photo, $destination, FILE_EXISTS_REPLACE)) {
            drupal_set_message('Unable to save image for R25 Location ' . $space_id, 'error');
            $photo_id = NULL;
          }
        }
        else {
          $photo_id = NULL;
        }
      }
      $room_info['photo_id'] = $photo_id;
      //$room_info['photo_id'] = $results['vals'][$results['index']['R25:LAYOUT_PHOTO_ID'][0]]['attributes']['XL:HREF'];
    }
  }
  return $room_info;
}

// retrieve the 25Live security group id given the security group name
function _stanford_r25_secgroup_id($secgroup) {
  $groupid = 0;
  if (!empty($secgroup)) {
    $results = _stanford_r25_api_call('secgroup', NULL, NULL, $secgroup);
    if (!empty($results['vals'][$results['index']['R25:SECURITY_GROUP_ID'][0]]['value'])) {
      $groupid = intval($results['vals'][$results['index']['R25:SECURITY_GROUP_ID'][0]]['value']);
    }
  }
  return $groupid;
}

// if using the custom event attributes, use the ids in the list to retrieve the name and type of
// the attribute fields
function _stanford_r25_update_event_attribute_fields($attr_list, $contact = FALSE) {
  $field_info = array();
  if (!empty($attr_list)) {
    $attrs = explode(",", $attr_list);
    foreach ($attrs as $attr) {
      $attr_id = trim($attr);
      if (substr($attr_id, -1) == '*') {
        $attr_id = substr($attr_id, 0, strlen($attr_id) - 1);
      }
      $results = _stanford_r25_api_call('evatrb', NULL, NULL, $attr_id);
      if ($results) {
        $field_info[$attr_id] = array(
          'name' => empty($results['index']['R25:ATTRIBUTE_NAME'][0]) ? NULL : $results['vals'][$results['index']['R25:ATTRIBUTE_NAME'][0]]['value'],
          'type' => empty($results['index']['R25:ATTRIBUTE_TYPE'][0]) ? NULL : $results['vals'][$results['index']['R25:ATTRIBUTE_TYPE'][0]]['value'],
          'contact' => $contact,
        );
      }
    }
  }
  return $field_info;
}

// delete a room from a Delete button on the edit form
function _stanford_r25_room_delete_submit($form, &$form_state) {
  _stanford_r25_room_config_delete_room($form_state['values']['machine_name']);
  $form_state['redirect'] = STANFORD_R25_MENU_PATH . '/list';
}

// delete a room configuration
function _stanford_r25_room_delete($form, &$form_state, $config = NULL) {
  $rooms = _stanford_r25_room_config_load();
  $form = array(
    'machine_name' => array('#type' => 'value', '#value' => $config),
    'label' => array(
      '#type' => 'value',
      '#value' => $rooms[$config]['display_name']
    )
  );
  $message = t('Are you sure you want to delete the configuration %label?', array('%label' => $rooms[$config]['display_name']));
  $caption = '<p>' . t('This action cannot be undone.') . '</p>';
  return confirm_form($form, filter_xss_admin($message), STANFORD_R25_MENU_PATH . '/list', filter_xss_admin($caption), t('Delete'));
}
