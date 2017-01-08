<?php

// the menu page callback function to build the main content for a calendar page
function _stanford_r25_calendar_page($room = NULL) {
  //$params = drupal_get_query_parameters();
  $params = array();
  foreach (drupal_get_query_parameters() as $key => $param) {
    $params[$key] = check_plain($param);
  }
  $rooms = _stanford_r25_room_config_load();
  $output = array();
  if (!empty($room) && isset($rooms[$room]['status'])) {
    $status = intval($rooms[$room]['status']);

    // make sure we have a valid room
    if ($status > STANFORD_R25_ROOM_STATUS_DISABLED) {
      // set some JavaScript variables to be used at the browser
      drupal_add_js(array('stanfordR25Room' => $room), 'setting');
      drupal_add_js(array('stanfordR25Status' => $status), 'setting');
      drupal_add_js(array('stanfordR25MaxHours' => $rooms[$room]['max_hours']), 'setting');
      $bookable = _stanford_r25_can_book($room);
      $can_book = ($bookable['can_book'] ? 1 : 0);
      drupal_add_js(array('stanfordR25Access' => $can_book), 'setting');
      drupal_add_js(array('stanfordR25DefaultView' => $rooms[$room]['default_view']), 'setting');
      if (!empty($params['view'])) {
        drupal_add_js(array('stanfordR25ParamView' => $params['view']), 'setting');
      }
      if (!empty($params['date'])) {
        drupal_add_js(array('stanfordR25ParamDate' => $params['date']), 'setting');
      }
      $multi_day = (!empty($rooms[$room]['multi_day'])) ? 1 : 0;
      drupal_add_js(array('stanfordR25MultiDay' => $multi_day), 'setting');
      $calendar_limit = array(
        'room' => $room,
        'month' => date('n'),
        'year' => date('Y') + 1
      );
      drupal_alter('stanford_r25_fullcalendar_limit', $calendar_limit);
      drupal_add_js(array('stanfordR25CalendarLimitMonth' => $calendar_limit['month']), 'setting');
      drupal_add_js(array('stanfordR25CalendarLimitYear' => $calendar_limit['year']), 'setting');
      if (intval($rooms[$room]['caltype']) == 1) {
        // if the calendar is from 25Live Publisher, output the appropriate divs and javascript files
        $output['control_div'] = array(
          '#type' => 'markup',
          '#markup' => '<div id="control-spud"></div>',
        );
        $output['calendar_div'] = array(
          '#type' => 'markup',
          '#markup' => '<div id="calendar-spud">Refresh browser if calendar does not appear.</div>',
        );
        drupal_add_js('https://25livepub.collegenet.com/scripts/spuds.js', array(
          'type' => 'external',
          'defer' => TRUE
        ));
        drupal_add_js(array('stanfordR25Spud' => $rooms[$room]['spud_name']), 'setting');
        drupal_add_js(drupal_get_path('module', 'stanford_r25') . '/stanford_r25_spud.js');
      }
      else {
        // if the calendar is FullCalendar, output links to the required javascript files and css
        drupal_add_css(libraries_get_path('fullcalendar') . '/fullcalendar.css');
        drupal_add_js(array('stanfordR25Timezone' => date_default_timezone()), 'setting');
        drupal_add_js(libraries_get_path('fullcalendar') . '/lib/moment.min.js');
        drupal_add_js(libraries_get_path('fullcalendar') . '/fullcalendar.min.js');
        // if we have the qtip tooltip library, add that
        if (libraries_get_path('jquery.qtip') !== FALSE && user_is_logged_in()) {
          drupal_add_css(libraries_get_path('jquery.qtip') . '/jquery.qtip.css');
          drupal_add_js(libraries_get_path('jquery.qtip') . '/jquery.qtip.js');
          drupal_add_js(array('stanfordR25Qtip' => 'qtip'), 'setting');
        }
        drupal_add_js(drupal_get_path('module', 'stanford_r25') . '/stanford_r25_fullcal.js');
        drupal_add_css(drupal_get_path('module', 'stanford_r25') . '/stanford_r25_selfserve.css');
        $output['calendar'] = array(
          '#type' => 'markup',
          '#markup' => '<div id="calendar"></div>',
        );
        if (isset($rooms[$room]['permalink']) && intval($rooms[$room]['permalink']) == 1) {
          $output['permalink'] = array(
            '#type' => 'markup',
            '#markup' => '<div id="permalink"></div>',
          );
        }
      }
    }
    else {
      $output['no_room'] = array(
        '#type' => 'markup',
        '#markup' => '<span>The requested room is not currently available.</span>',
      );
    }
  }
  else {
    $output = MENU_NOT_FOUND;
  }
  return $output;
}

// the reservation form callback is called by the reservation block
function stanford_r25_reservation($form, &$form_state, $room = '', $max_headcount = 5, $contact_info = '', $external_acct = FALSE) {
  // keep the roomid for later processing
  $form['stanford_r25_booking_roomid'] = array(
    '#type' => 'hidden',
    '#value' => $room,
  );
  $rooms = _stanford_r25_room_config_load();
  // if the room only accepts tentative bookings, then put up a message to that effect
  if (!empty($rooms[$room]['status']) && intval($rooms[$room]['status']) == STANFORD_R25_ROOM_STATUS_TENTATIVE) {
    $form['stanford_r25_booking_tentative'] = array(
      '#type' => 'markup',
      '#markup' => "<p>This room only accepts tentative reservations which must be approved by the room's administrator.</p>",
    );
  }
  // use the Drupal date popup for date and time picking
  $form['stanford_r25_booking_date'] = array(
    '#type' => 'date_popup',
    '#default_value' => format_date(time(), 'custom', 'Y-m-d H:i:s'),
    '#date_type' => DATE_DATETIME,
    '#date_timezone' => date_default_timezone(),
    '#date_format' => 'Y-m-d h:i a',
    '#date_increment' => 30,
    '#date_year_range' => '-0:+1',
    '#required' => TRUE,
    '#title' => 'Start Date/Time',
  );

  if (empty($rooms[$room]['multi_day'])) {
    // default booking duration is limited to 2 hours in 30 minute increments
    // but a room can have a different value
    $max_hours = 2;
    if (!empty($rooms[$room]['max_hours'])) {
      $max_hours = intval($rooms[$room]['max_hours']);
      if ($max_hours == 0) {
        $max_hours = 24;
      }
    }
    $hours_array = array();
    if ($max_hours > 2) {
      for ($i = 0; $i < $max_hours; $i++) {
        $hstr = '';
        if ($i == 0) {
          $hours_array[] = '30 minutes';
          $hours_array[] = '1 hour';
        }
        else {
          $hours_array[] = strval($i) . '.5 hours';
          $hours_array[] = strval($i + 1) . ' hours';
        }
      }
    }
    else {
      $hours_array[0] = '30 minutes';
      $hours_array[1] = '60 minutes';
      $hours_array[2] = '90 minutes';
      $hours_array[3] = '120 minutes';
    }
    $form['stanford_r25_booking_duration'] = array(
      '#type' => 'select',
      '#title' => t('Duration'),
      '#options' => $hours_array,
      '#default_value' => 0,
      '#required' => TRUE,
    );
  }
  else {
    $max_hours = '';
    $form['stanford_r25_booking_enddate'] = array(
      '#type' => 'date_popup',
      '#default_value' => format_date(time(), 'custom', 'Y-m-d H:i:s'),
      '#date_type' => DATE_DATETIME,
      '#date_timezone' => date_default_timezone(),
      '#date_format' => 'Y-m-d h:i a',
      '#date_increment' => 30,
      '#date_year_range' => '-0:+1',
      '#required' => TRUE,
      '#title' => 'End Date/Time',
    );
  }
  // max headcount for a room comes from parameter passed to the function
  $form['stanford_r25_booking_headcount'] = array(
    '#type' => 'select',
    '#title' => t('Headcount'),
    '#options' => array(),
    '#required' => TRUE,
  );
  // add to the select list for the number of possible headcounts
  for ($i = 1; $i < $max_headcount + 1; $i++) {
    $form['stanford_r25_booking_headcount']['#options'][] = strval($i);
  }
  // every booking needs some reason text
  $form['stanford_r25_booking_reason'] = array(
    '#type' => 'textfield',
    '#title' => t('Reason'),
    '#required' => TRUE,
    '#maxlength' => 40,
  );

  // check for event attribute fields, and build 'em
  if (!empty($rooms[$room]['event_attribute_fields'])) {
    foreach ($rooms[$room]['event_attribute_fields'] as $attr_id => $attr_info) {
      switch ($attr_info['type']) {
        case 'S':
          $field_type = 'textfield';
          break;
        case 'B':
          $field_type = 'checkbox';
          break;
        case 'X':
          $field_type = 'textarea';
          break;
        default:
          $field_type = '';
      }
      if (!empty($field_type)) {
        $form['stanford_r25_booking_attr' . $attr_id] = array(
          '#type' => $field_type,
          '#title' => $attr_info['name'],
        );
      }
    }
  }

  if (!empty($rooms[$room]['contact_attr_field'])) {
    foreach ($rooms[$room]['contact_attr_field'] as $attr_id => $attr_info) {
      switch ($attr_info['type']) {
        case 'S':
          $field_type = 'textfield';
          break;
        case 'B':
          $field_type = 'checkbox';
          break;
        case 'X':
          $field_type = 'textarea';
          break;
        default:
          $field_type = '';
      }
      if (!empty($field_type)) {
        $form['stanford_r25_contact_' . $attr_id] = array(
          '#type' => $field_type,
          '#title' => $attr_info['name'],
          '#default_value' => $contact_info,
        );
      }
    }
  }

  if (!empty($external_acct) && is_array($external_acct)) {
    if (!empty($external_acct['R25_EXTERNAL_DISPLAYNAME']) &&
      is_string($external_acct['R25_EXTERNAL_DISPLAYNAME'])
    ) {
      $form['external_username'] = array(
        '#type' => 'hidden',
        '#value' => $external_acct['R25_EXTERNAL_DISPLAYNAME'],
      );
    }
    if (!empty($external_acct['R25_EXTERNAL_MAIL']) &&
      is_string($external_acct['R25_EXTERNAL_MAIL'])
    ) {
      $form['external_usermail'] = array(
        '#type' => 'hidden',
        '#value' => $external_acct['R25_EXTERNAL_MAIL'],
      );
    }
  }
  // submit button, d'uh
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Reserve'),
  );
  // display some reservation instructions if available
  if (!empty($rooms[$room]['override_booking_instructions']['value'])) {
    $booking_instr = $rooms[$room]['override_booking_instructions'];
  }
  else {
    $booking_instr = variable_get('stanford_r25_booking_instructions', array(
      'value' => '',
      'format' => NULL
    ));
  }
  if (!empty($booking_instr['value'])) {
    $booking_instr['value'] = str_replace('[max_duration]', $max_hours, $booking_instr['value']);
  }
  $form['r25_instructions'] = array(
    '#type' => 'markup',
    '#markup' => check_markup($booking_instr['value'], $booking_instr['format']),
  );

  return $form;

}

/**
 * // some date and time formatting stuff - taking input from form date/time and duration fields
 * // and returning start and end times in W3C format to pass to the 25Live web services api.
 * function _stanford_r25_format_date_and_time($date, $booking_duration, $max_hours)
 * {
 * //$date = DateTime::createFromFormat("Y-m-d g:i a", $booking_date['date'] . ' ' . $booking_date['time']);
 * //$date_errors = DateTime::getLastErrors();
 * // don't allow reservations more than 1/2 hour in the past. we're not a time machine.
 * //if (date_timestamp_get($date) < (time() - 1800)) {
 * //    $date = false;
 * //}
 * //if (!$date || $date_errors["warning_count"] > 0 || $date_errors["error_count"] > 0) {
 * //    return false;
 * // }
 * $duration = intval($booking_duration);
 * if ($duration < 0 || $duration > (($max_hours*2)-1)) return false;
 * $duration = ($duration * 30) + 30;
 * return array('day' => $date->format('Y-m-d'), 'start' => $date->format(DateTime::W3C),
 * 'end' => $date->add(new DateInterval('PT' . $duration . 'M'))->format(DateTime::W3C));
 * }
 **/

// reservation form validation - formats the date strings and stores them in form 'storage' for the submit routine
function stanford_r25_reservation_validate($form, &$form_state) {

  $booking_info = array();
  $rooms = _stanford_r25_room_config_load();
  if (empty($form_state['input']['stanford_r25_booking_roomid']) || !isset($rooms[$form_state['input']['stanford_r25_booking_roomid']])) {
    form_set_error('stanford_r25_booking_roomid', 'Invalid room id requested.');
    return;
  }
  else {
    $booking_info['room'] = $rooms[$form_state['input']['stanford_r25_booking_roomid']];
  }

  $can_book = _stanford_r25_can_book($booking_info['room']['machine_name']);
  if (!$can_book['can_book']) {
    form_set_error('stanford_r25_booking_reason', 'User does not have permission to book rooms.');
    return;
  }

  // make sure we have a valid date
  $booking_date = $form_state['input']['stanford_r25_booking_date'];
  $date = DateTime::createFromFormat("Y-m-d g:i a", $booking_date['date'] . ' ' . $booking_date['time']);
  $date_errors = DateTime::getLastErrors();
  if (!$date || $date_errors["warning_count"] > 0 || $date_errors["error_count"] > 0) {
    form_set_error('stanford_r25_booking_date', 'An invalid date was supplied.');
    return;
  }
  // don't allow reservations more than 1/2 hour in the past. we're not a time machine.
  if (date_timestamp_get($date) < (time() - 1800)) {
    form_set_error('stanford_r25_booking_date', 'A reservation in the past was requested. This isn\'t a time machine!');
    return;
  }

  $end_date = 0;
  if (!empty($form_state['input']['stanford_r25_booking_enddate'])) {
    $booking_end_date = $form_state['input']['stanford_r25_booking_enddate'];
    $end_date = DateTime::createFromFormat("Y-m-d g:i a", $booking_end_date['date'] . ' ' . $booking_end_date['time']);
    $date_errors = DateTime::getLastErrors();
    if (!$end_date || $date_errors["warning_count"] > 0 || $date_errors["error_count"] > 0) {
      form_set_error('stanford_r25_booking_enddate', 'An invalid end date was supplied.');
      return;
    }
    if (date_timestamp_get($end_date) <= date_timestamp_get($date)) {
      form_set_error('stanford_r25_booking_enddate', 'The end date may not be before the start date.');
      return;
    }
  }

  // make sure date isn't blacked out if room checks for that
  if ($rooms[$form_state['input']['stanford_r25_booking_roomid']]['honor_blackouts'] == 1) {
    if (_stanford_r25_date_blacked_out(date_timestamp_get($date)) ||
      (!empty($end_date) && _stanford_r25_date_blacked_out(date_timestamp_get($end_date)))
    ) {
      form_set_error('stanford_r25_booking_date', 'This room is unavailable for reservation on the requested date. ' .
        'The room may only be reserved until the end of the current quarter. Please see your department ' .
        'administrator for more information.');
      return;
    }
  }

  // build 25Live date strings
  if (!empty($end_date)) {
    $date_strs = array(
      'day' => $date->format('Y-m-d'),
      'start' => $date->format(DateTime::W3C),
      'end' => $end_date->format(DateTime::W3C),
    );
  }
  else {
    $duration = intval($form_state['input']['stanford_r25_booking_duration']);
    if ($duration < 0 || $duration > (($booking_info['room']['max_hours'] * 2) - 1)) {
      form_set_error('stanford_r25_booking_duration', 'Invalid duration was submitted.');
      return;
    }
    $duration = ($duration * 30) + 30;
    $date_strs = array(
      'day' => $date->format('Y-m-d'),
      'start' => $date->format(DateTime::W3C),
      'end' => $date->add(new DateInterval('PT' . $duration . 'M'))
        ->format(DateTime::W3C)
    );
  }

  //$date_strs = _stanford_r25_format_date_and_time($form_state['input']['stanford_r25_booking_date'],
  //    $form_state['input']['stanford_r25_booking_duration'],$booking_info['room']['max_hours']);
  //if ($date_strs === false) {
  //    form_set_error('stanford_r25_booking_time', 'Invalid date and time or duration submitted.');
  //} else {
  $booking_info['dates'] = $date_strs;
  //}
  $form_state['storage']['stanford_r25_booking'] = $booking_info;
}

// submits the reservation request to 25Live webservices
function stanford_r25_reservation_submit($form, &$form_state) {
  // make sure the user has access, that the needed information is available, and the room is bookable
  if (empty($form_state['storage']['stanford_r25_booking']['dates']) || empty($form_state['storage']['stanford_r25_booking']['room'])) {
    drupal_set_message('Insufficient booking information was provided.', 'error');
    unset($form_state['storage']['stanford_r25_booking']);
    return;
  }
  $event_state = intval($form_state['storage']['stanford_r25_booking']['room']['status']);
  if ($event_state < STANFORD_R25_ROOM_STATUS_TENTATIVE || $event_state > STANFORD_R25_ROOM_STATUS_CONFIRMED) {
    drupal_set_message('This room may not be reserved through this website.', 'error');
    unset($form_state['storage']['stanford_r25_booking']);
    return;
  }

  $can_book = _stanford_r25_can_book($form_state['storage']['stanford_r25_booking']['room']['machine_name']);
  if (!$can_book['can_book']) {
    drupal_set_message('You do not have permission to book this room.', 'error');
    unset($form_state['storage']['stanford_r25_booking']);
    return;
  }

  $mail_list = '';
  $todo_insert = '';
  if ($event_state == STANFORD_R25_ROOM_STATUS_TENTATIVE &&
    !empty($form_state['storage']['stanford_r25_booking']['room']['approver_secgroup_id'])
  ) {
    $approver_list =
      _stanford_r25_security_group_emails($form_state['storage']['stanford_r25_booking']['room']['approver_secgroup_id']);
    if (!empty($approver_list)) {
      $todo_str = file_get_contents(drupal_get_path('module', 'stanford_r25') . '/stanford_r25_reserve_todo.xml');
      foreach ($approver_list as $key => $value) {
        $todo_temp = str_replace('[r25_start_date_time]', $form_state['storage']['stanford_r25_booking']['dates']['start'], $todo_str);
        $todo_temp = str_replace('[r25_approver_id]', $key, $todo_temp);
        $todo_temp = str_replace('[r25_credential_id]', variable_get('stanford_r25_credential_contact_id', ''), $todo_temp);
        $todo_insert .= $todo_temp;
        if (!empty($mail_list)) {
          $mail_list .= ', ';
        }
        $mail_list .= $value;
      }
    }
  }

  $attr_insert = '';
  $attr_str = file_get_contents(drupal_get_path('module', 'stanford_r25') . '/stanford_r25_reserve_attr.xml');
  $room = $form_state['storage']['stanford_r25_booking']['room'];
  if (!empty($room['event_attribute_fields'])) {
    foreach ($room['event_attribute_fields'] as $key => $value) {
      if (!empty($form_state['values']['stanford_r25_booking_attr' . $key])) {
        $attr_temp = str_replace('[r25_attr_id]', $key, $attr_str);
        $attr_temp = str_replace('[r25_attr_type]', $value['type'], $attr_temp);
        $attr_temp = str_replace('[r25_attr_value]', $form_state['values']['stanford_r25_booking_attr' . $key], $attr_temp);
        $attr_insert .= $attr_temp;
      }
    }
  }
  if (!empty($room['contact_attr_field'])) {
    foreach ($room['contact_attr_field'] as $key => $value) {
      if (!empty($form_state['values']['stanford_r25_contact_' . $key])) {
        $attr_temp = str_replace('[r25_attr_id]', $key, $attr_str);
        $attr_temp = str_replace('[r25_attr_type]', $value['type'], $attr_temp);
        $attr_temp = str_replace('[r25_attr_value]', $form_state['values']['stanford_r25_contact_' . $key], $attr_temp);
        $attr_insert .= $attr_temp;
      }
    }
  }

  // get the XML template for creating an event and replace tokens with data for this reservation
  $event_state = $event_state - 1;
  $xml_file = '/stanford_r25_reserve.xml';
  $xml = file_get_contents(drupal_get_path('module', 'stanford_r25') . $xml_file); //'/stanford_r25_reserve.xml');
  $xml = str_replace('[r25_event_name]', $form_state['values']['stanford_r25_booking_reason'], $xml);
  $xml = str_replace('[r25_parent_id]', variable_get('stanford_r25_parent_event_id', 'unknown'), $xml);
  $xml = str_replace('[r25_event_type]', variable_get('stanford_r25_event_type', 'unknown'), $xml);
  $xml = str_replace('[r25_event_state]', $event_state, $xml);
  $xml = str_replace('[r25_organization_id]', variable_get('stanford_r25_org_id', 'unknown'), $xml);
  $xml = str_replace('[r25_expected_headcount]', $form_state['complete form']['stanford_r25_booking_headcount']['#options'][$form_state['values']['stanford_r25_booking_headcount']], $xml);
  $xml = str_replace('[r25_start_date_time]', $form_state['storage']['stanford_r25_booking']['dates']['start'], $xml);
  $xml = str_replace('[r25_end_date_time]', $form_state['storage']['stanford_r25_booking']['dates']['end'], $xml);
  $xml = str_replace('[r25_space_id]', $form_state['storage']['stanford_r25_booking']['room']['space_id'], $xml);
  $xml = str_replace('[r25_todo]', $todo_insert, $xml);
  $xml = str_replace('[r25_attr]', $attr_insert, $xml);
  global $user;
  $res_username = '';
  $res_usermail = '';
  if ($user->uid == 0) {
    if (!empty($form_state['input']['external_username'])) {
      $res_username = $form_state['input']['external_username'];
    }
    if (!empty($form_state['input']['external_usermail'])) {
      $res_usermail = $form_state['input']['external_usermail'];
    }
  }
  else {
    $res_username = $user->name;
    $res_usermail = $user->mail;
  }
  $xml = str_replace('[r25_created_by]', '<span>Self service reservation made by ' . $res_username . ' - <a href="mailto:' . $res_usermail . '">click to contact by email.</a></span>', $xml);

  // send the request to our api function
  $result = _stanford_r25_api_call('reserve', NULL, NULL, $xml);

  // check the results to see if our reservation attempt was successful
  $success = FALSE;
  if ($result) {
    // a successful return with no status message is assumed to be a success since that's how the webservices api works. go figure.
    // if we use the setting that returns a positive return code for success, then other information is missing.
    if (empty($result['index']['R25:MSG_ID'][0])) {
      // check if the result has the location and time we requested
      if (!empty($result['index']['R25:SPACE_ID'][0]) &&
        $result['vals'][$result['index']['R25:SPACE_ID'][0]]['value'] == $form_state['storage']['stanford_r25_booking']['room']['space_id'] &&
        !empty($result['index']['R25:EVENT_START_DT'][0]) &&
        $result['vals'][$result['index']['R25:EVENT_START_DT'][0]]['value'] == $form_state['storage']['stanford_r25_booking']['dates']['start'] &&
        !empty($result['index']['R25:EVENT_END_DT'][0]) &&
        $result['vals'][$result['index']['R25:EVENT_END_DT'][0]]['value'] == $form_state['storage']['stanford_r25_booking']['dates']['end']
      ) {
        $success = TRUE;
      }
    }
    else {
      // even though we should not see a success code, we do want to check if we got a failure code, which is anything but the
      // two defined success codes.
      $msg_index = $result['index']['R25:MSG_ID'][0];
      if (!empty($result['vals'][$msg_index]['value'])) {
        if ($result['vals'][$msg_index]['value'] === 'EV_I_SAVE' ||
          $result['vals'][$msg_index]['value'] === 'EV_I_CREATED'
        ) {
          $success = TRUE;
        }
      }
    }
  }
  if ($success) {

    // if the booking was successful, format and display a message to that effect
    $date = DateTime::createFromFormat(DATE_W3C, $form_state['storage']['stanford_r25_booking']['dates']['start']);
    $state = intval($result['vals'][$result['index']['R25:STATE'][0]]['value']);
    $msg = $form_state['storage']['stanford_r25_booking']['room']['display_name'] . ' has a <b>' . $result['vals'][$result['index']['R25:STATE_NAME'][0]]['value'] . '</b> reservation for "' . $form_state['values']['stanford_r25_booking_reason'] . '" on  ' . $date->format("l, F j, Y g:i a") . '.';
    if (intval($result['vals'][$result['index']['R25:STATE'][0]]['value']) == 1) {
      $msg .= ' The room administrator will confirm or deny your request.';
    }
    drupal_set_message($msg);

    $estimated_charge = 0;
    $billable = FALSE;
    $eventid = $result['vals'][$result['index']['R25:EVENT_ID'][0]]['value'];
    if (!empty($form_state['storage']['stanford_r25_booking']['room']['auto_billing_code'])) {
      $bill_code = $form_state['storage']['stanford_r25_booking']['room']['auto_billing_code'];
      $billable = TRUE;
      drupal_alter('stanford_r25_isbillable', $billable);
      if ($billable) {
        $billing_get = _stanford_r25_api_call('billing-get', NULL, NULL, $eventid);
        $billing_xml = $billing_get['raw-xml'];
        $est_ptr = strpos($billing_xml, 'status="est"');
        $billing_xml = substr($billing_xml, 0, $est_ptr) . 'status="mod"' . substr($billing_xml, $est_ptr + 12);
        $space_code = strpos($billing_xml, $form_state['storage']['stanford_r25_booking']['room']['space_id']);
        $bill_tmp = substr($billing_xml, 0, $space_code);
        $est_ptr = strrpos($bill_tmp, 'status="est"');
        $billing_xml = substr($billing_xml, 0, $est_ptr) . 'status="mod"' . substr($billing_xml, $est_ptr + 12);
        $est_ptr = strpos($billing_xml, '<r25:rate_group_id/>', $space_code);
        $billing_xml = substr($billing_xml, 0, $est_ptr) . '<r25:rate_group_id>' . $bill_code . '</r25:rate_group_id>' .
          substr($billing_xml, $est_ptr + 20);

        $history_ptr = strpos($billing_xml, '<r25:history_type_id>4');
        $est_ptr = strrpos(substr($billing_xml, 0, $history_ptr), '"est"');
        $billing_xml = substr($billing_xml, 0, $est_ptr) . '"mod"' . substr($billing_xml, $est_ptr + 5);
        $hist_dt_ptr1 = strpos($billing_xml, '<r25:history_dt>', $history_ptr);
        $hist_dt_ptr2 = strpos($billing_xml, '</r25:history_dt>', $history_ptr);
        $billing_xml = substr($billing_xml, 0, $hist_dt_ptr1 + 16) .
          $form_state['storage']['stanford_r25_booking']['dates']['start'] .
          substr($billing_xml, $hist_dt_ptr2);
        $result = _stanford_r25_api_call('billing-put', NULL, NULL, $billing_xml, $eventid);
        if ($result) {
          if (!empty($result['index']['R25:BILL_ITEM_TYPE_NAME']) &&
            is_array($result['index']['R25:BILL_ITEM_TYPE_NAME'])
          ) {
            foreach ($result['index']['R25:BILL_ITEM_TYPE_NAME'] as $key => $value) {
              if (!empty($result['vals'][$value]['value']) &&
                $result['vals'][$value]['value'] === 'GRAND TOTAL'
              ) {
                if (!empty($result['index']['R25:TOTAL_CHARGE'][$key])) {
                  $key2 = $result['index']['R25:TOTAL_CHARGE'][$key];
                  if (!empty($result['vals'][$key2]['value'])) {
                    $estimated_charge = intval($result['vals'][$key2]['value']);
                  }
                }
                break;
              }
            }
          }
        }
      }
    }

    // send an email about the booking if mail list is set
    if (!empty($mail_list) && !empty($form_state['storage']['stanford_r25_booking']['room']['email_list'])) {
      $mail_list .= ', ';
    }
    $mail_list .= $form_state['storage']['stanford_r25_booking']['room']['email_list'];
    $body = array();
    $body[] = "A " . $result['vals'][$result['index']['R25:STATE_NAME'][0]]['value'] . " reservation has been made";
    $subject = '';
    if ($state == 1) {
      // this is the email for a tentative booking
      $subject = 'Room Reservation Request - ACTION REQUIRED';
      $body[0] .= ' requiring your approval.';
      $body[] = 'You may view this request in 25Live and confirm or deny it at this link (requires you first be logged in to 25Live): ';
      $body[] = 'https://25live.collegenet.com/stanford/#details&obj_type=event&obj_id=' . $result['vals'][$result['index']['R25:EVENT_ID'][0]]['value'];
      $body[] = '';
    }
    else {
      if ($state == 2) {
        // this is the email for a confirmed booking
        $subject = 'Room Reservation';
        $body[0] .= '.';
        $body[] = 'View the reservation at: https://25live.collegenet.com/stanford/#details&obj_type=event&obj_id=' . $result['vals'][$result['index']['R25:EVENT_ID'][0]]['value'];
      }
    }
    $body[] = "Room: " . $form_state['storage']['stanford_r25_booking']['room']['display_name'];
    if (!empty($form_state['values']['stanford_r25_booking_duration'])) {
      $body[] = "Date: " . $date->format("l, F j, Y g:i a");
      $duration = (intval($form_state['values']['stanford_r25_booking_duration']) * 30) + 30;
      if ($duration > 120) {
        $body[] = 'Duration: ' . $duration / 60 . ' hours';
      }
      else {
        $body[] = 'Duration: ' . $duration . ' minutes';
      }
    }
    else {
      $body[] = "Start Date: " . $date->format("l, F j, Y g:i a");
      $enddate = DateTime::createFromFormat(DATE_W3C, $form_state['storage']['stanford_r25_booking']['dates']['end']);
      $body[] = "End Date: " . $enddate->format("l, F j, Y g:i a");
    }
    $body[] = "Reason: " . $form_state['values']['stanford_r25_booking_reason'];
    $body[] = "Requested by: " . $res_username . " " . $res_usermail;
    if ($estimated_charge > 0) {
      $body[] = "Estimated Fee: $" . $estimated_charge;
    }
    $params = array(
      'body' => $body,
      'subject' => $subject
    );
    drupal_mail('stanford_r25', $eventid, $mail_list, language_default(), $params);
    if (!empty($room['postprocess_booking']) && !empty($res_usermail)) {
      $form_state['storage']['stanford_r25_postprocess'] = array(
        'room' => $form_state['storage']['stanford_r25_booking']['room'],
        'dates' => $form_state['storage']['stanford_r25_booking']['dates'],
        'mailto' => $res_usermail,
        'event_name' => $form_state['values']['stanford_r25_booking_reason'],
        'eventid' => $eventid,
        'est_charge' => $estimated_charge
      );
    }
  }
  else {
    // display a message if the booking failed
    drupal_set_message('The system was unable to book your room. This may be because of a time conflict with another meeting, or because someone else booked it first or because of problems communicating with 25Live. Please try again.', 'error');
    $body = array();
    $event_id = 0;
    if (!empty($result['index']['R25:EVENT_ID'][0]) && !empty($result['vals'][$result['index']['R25:EVENT_ID'][0]]['value'])) {
      $event_id = $result['vals'][$result['index']['R25:EVENT_ID'][0]]['value'];
      $body[] = 'failed reservation at: https://25live.collegenet.com/stanford/#details&obj_type=event&obj_id=' . $event_id;
      _stanford_r25_api_call('delete', NULL, NULL, $event_id);
    }
  }
  // remove the storage we set up in validate
  unset($form_state['storage']['stanford_r25_booking']);
}

function _stanford_r25_cancel_reservation($form, &$form_state, $room_id, $event_id, $start) {
    $op = 'cancel';
    $opout = 'cancellation';

    $cur = current_path();
    if (strpos($cur,'confirm') !== false) {
        $op = 'confirm';
        $opout = 'confirmation';
    }
  $rooms = _stanford_r25_room_config_load();
  $result = _stanford_r25_user_can_cancel_or_confirm($room_id, $event_id, $op);
  if (!$result) {
    drupal_access_denied();
  }
  $title = '';
  if (!empty($result['vals'][$result['index']['R25:EVENT_NAME'][0]]['value'])) {
    $title = $result['vals'][$result['index']['R25:EVENT_NAME'][0]]['value'];
  }
  $event_count = 0;
  if (!empty($result['index']['R25:RESERVATION_START_DT']) &&
    is_array($result['index']['R25:RESERVATION_START_DT'])
  ) {
    $event_count = count($result['index']['R25:RESERVATION_START_DT']);
  }

  if (empty($form_state['input'])) {
    $msg = 'Do you want to ' . $op . ' reservation "';
    if (!empty($title)) {
      $msg .= check_plain($title);
    }
    else {
      $msg .= $event_id;
    }
    $msg .= '"';
    if (!empty($rooms[$room_id]['display_name'])) {
      $msg .= ' in room ' . $rooms[$room_id]['display_name'];
    }
    if (!empty($start)) {
      $startdate = DateTime::createFromFormat(DATE_W3C, $start);
      $msg .= ' for ' . $startdate->format("l, F j, Y g:i a");
    }
    $msg .= '? <br />';
    $form['room_id'] = array(
      '#type' => 'hidden',
      '#value' => $room_id,
    );
    $form['event_id'] = array(
      '#type' => 'hidden',
      '#value' => $event_id,
    );
    $form['really'] = array(
      '#markup' => t($msg),
    );

    if ($event_count > 1) {
        $msg_text = 'This reservation is part of a series. ' . ucfirst($opout) . ' will apply to all dates.<br />';
        if (!empty($result['index']['R25:RESERVATION_START_DT']) &&
            is_array($result['index']['R25:RESERVATION_START_DT'])) {
            $msg_text .= 'Reservation dates: <br/>';
            foreach ($result['index']['R25:RESERVATION_START_DT'] as $key => $value) {
                if (!empty($result['vals'][$value]['value'])) {
                    $s_date = DateTime::createFromFormat(DATE_W3C, $result['vals'][$value]['value']);
                    $msg_text .= $s_date->format("l, F j, Y g:i a") . '<br />';
                }
            }
            $msg_text .= '<br />';
        }
        $form['markup2'] = array(
            '#markup' => t($msg_text),
        );
        if ($op == 'cancel') {
            $form['series'] = array(
                '#type' => 'radios',
                '#default_value' => 1,
                '#options' => array(
                    1 => 'Cancel this occurrence',
                    2 => 'Cancel entire series'
                ),
            );
        } else {
            $form['series'] = array(
                '#type' => 'hidden',
                '#value' => 2,
            );
        }
    }
    else {
      $form['series'] = array(
        '#type' => 'hidden',
        '#value' => 2,
      );
    }
  }
  else {
    $xml_string = $result['raw-xml'];
    $find_str = 'status="est"';
    if ($form_state['input']['series'] == 1) {
      $reservation_id = 0;
      if (!empty($result['index']['R25:RESERVATION_START_DT']) &&
        is_array($result['index']['R25:RESERVATION_START_DT'])
      ) {
        foreach ($result['index']['R25:RESERVATION_START_DT'] as $key => $value) {
          if (!empty($result['vals'][$value]['value']) &&
            $start == $result['vals'][$value]['value']
          ) {
            $reskey = $result['index']['R25:RESERVATION_ID'][$key];
            $reservation_id = $result['vals'][$reskey]['value'];
            break;
          }
        }
        if ($reservation_id > 0) {
          $resptr1 = strpos($xml_string, '<r25:reservation_id>' . $reservation_id);
          if ($resptr1 !== FALSE) {
            $pos = strpos($xml_string, $find_str);
            if ($pos !== FALSE) {
              $xml_string = substr_replace($xml_string, 'status="mod"', $pos, strlen($find_str));
            }
            $proptr = strpos($xml_string, '<r25:profile');
            if ($proptr !== FALSE) {
              $pos = strpos($xml_string, $find_str, $proptr);
              if ($pos !== FALSE) {
                $xml_string = substr_replace($xml_string, 'status="mod"', $pos, strlen($find_str));
              }
            }
            $temp1 = substr($xml_string, 0, $resptr1);
            $resptr2 = strrpos($temp1, $find_str);
            if ($resptr2 !== FALSE) {
              $xml_string = substr($xml_string, 0, $resptr2) . 'status="mod"' . substr($xml_string, $resptr2 + 12);
            }
            $resptr2 = strpos($xml_string, '<r25:reservation_state>', $resptr1);
            $resptr3 = strpos($xml_string, '</r25:reservation_state>', $resptr1);
            if ($resptr2 !== FALSE && $resptr3 !== FALSE) {
              $xml_string = substr_replace($xml_string, '<r25:reservation_state>99</r25:reservation_state>', $resptr2, $resptr3 + 24 - $resptr2);
            }
          }
        }
      }
    }
    else {
      if ($form_state['input']['series'] == 2) {
        $pos = strpos($xml_string, $find_str);
        if ($pos !== FALSE) {
          $xml_string = substr_replace($xml_string, 'status="mod"', $pos, strlen($find_str));
        }
        $pos1 = strpos($xml_string, '<r25:state>');
        $pos2 = strpos($xml_string, '</r25:state>');
        if ($pos1 !== FALSE && $pos2 !== FALSE) {
            $new_state = ($op == 'confirm') ? '2' : '99';
          $xml_string = substr_replace($xml_string, '<r25:state>' . $new_state . '</r25:state>', $pos1, $pos2 + 12 - $pos1);
        }
      }
    }

    _stanford_r25_api_call('event-put', NULL, NULL, $xml_string, $event_id);

    if ($rooms[$room_id]['email_confirms_and_cancels']) {
      $secgroup_id = '';
      if (!empty($rooms[$room_id]['approver_secgroup_id'])) {
        $secgroup_id = $rooms[$room_id]['approver_secgroup_id'];
      }
      $additional = '';
      if (!empty($rooms[$room_id]['email_list'])) {
        $additional = $rooms[$room_id]['email_list'];
      }
      $email_list = _stanford_r25_build_event_email_list($result, $secgroup_id, $additional);
      global $user;
      $body = array();
      $body[] = 'A Room Reservation ' . $opout . ' request was sent by ' . $user->name;
      $body[] =' for "' . $title . '" in room ' . $rooms[$room_id]['display_name'];
      $startdate = DateTime::createFromFormat(DATE_W3C, $start);
      if ($event_count > 1) {
        if ($form_state['input']['series'] == 1) {
          $body[] = ' for the instance on ' . $startdate->format("l, F j, Y g:i a");
        }
        else {
          if ($form_state['input']['series'] == 2) {
            $body[] = ' for the entire series including ' . $startdate->format("l, F j, Y g:i a");
          }
        }
      }
      else {
        $body[] = ' for the reservation starting ' . $startdate->format("l, F j, Y g:i a");
      }
      $subject = 'Room reservation ' . $opout;
      $params = array(
        'body' => $body,
        'subject' => $subject
      );
      drupal_mail('stanford_r25', $event_id, $email_list, language_default(), $params);
    }
    drupal_goto('/r25/' . $room_id . '/calendar');
  }
    $optext = ucfirst($op);
  $form = confirm_form($form, $optext . ' Reservation', 'r25/' . $room_id . '/calendar', NULL, $optext . ' Reservation', 'Go Back to Calendar', 'cancel_res');
  return $form;
}

// build a comma-delimited string of email addresses associated with a reservation
function _stanford_r25_build_event_email_list($results, $secgroup_id, $extra_list) {
  global $user;

  // get a list of email addresses for approvers for the event's room's security group
  $mail_array = _stanford_r25_security_group_emails($secgroup_id);

  // add on any extra email addresses for the room
  if (!empty($extra_list)) {
    $extras = explode(',', $extra_list);
    $mail_array = array_merge($mail_array, $extras);
  }

  // add the current user to the list
  if (!empty($user->uid) && $user->uid > 1 && !empty($user->mail)) {
    $mail_array[] = $user->mail;
  }

  // if the event was not done with quickbook, add the scheduler's email to the list
  // if the event *was* done with quickbook, pull the scheduler's email id from the event text
  $quickbook_id = intval(variable_get('stanford_r25_credential_contact_id', '0'));
  $quickbook = FALSE;
  if (!empty($results['index']['R25:ROLE_NAME']) && is_array($results['index']['R25:ROLE_NAME'])) {
    foreach ($results['index']['R25:ROLE_NAME'] as $key => $value) {
      if ($results['vals'][$value]['value'] == 'Scheduler') {
        if ($results['vals'][$results['index']['R25:CONTACT_ID'][$key]]['value'] == $quickbook_id) {
          $quickbook = TRUE;
        }
        else {
          $mail_array[] = $results['vals'][$results['index']['R25:EMAIL'][$key]]['value'];
        }
      }
    }
  }
  if ($quickbook) {
    if (!empty($results['index']['R25:TEXT_TYPE_NAME']) && is_array($results['index']['R25:TEXT_TYPE_NAME'])) {
      foreach ($results['index']['R25:TEXT_TYPE_NAME'] as $key => $value) {
        if ($results['vals'][$value]['value'] == 'Description') {
          $desc = $results['vals'][$results['index']['R25:TEXT'][$key]]['value'];
          if (strpos($desc, 'mailto:', 0) !== FALSE) {
            $mailto_start = strpos($desc, 'mailto:', 0) + 7;
            $mailto_end = strpos($desc, '"', $mailto_start);
            $mail_array[] = substr($desc, $mailto_start, $mailto_end - $mailto_start);
          }
        }
      }
    }
  }

  // get rid of duplicate email addresses
  $mail_array = array_unique($mail_array);

  // return the result as a string
  return implode(', ', $mail_array);
}

function _stanford_r25_confirm_reservation($form, &$form_state, $room_id, $event_id, $start, $title, $event_count) {
  $xyz = 1;
}