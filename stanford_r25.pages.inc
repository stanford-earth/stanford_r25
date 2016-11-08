<?php

// the menu page callback function to build the main content for a calendar page
function _stanford_r25_calendar_page($room = null)
{
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
            $can_book = (user_access('book r25 rooms') ? 1 : 0);
            drupal_add_js(array('stanfordR25Access' => $can_book), 'setting');
            drupal_add_js(array('stanfordR25DefaultView' => $rooms[$room]['default_view']), 'setting');
            if (!empty($params['view'])) {
                drupal_add_js(array('stanfordR25ParamView' => $params['view']), 'setting');
            }
            if (!empty($params['date'])) {
                drupal_add_js(array('stanfordR25ParamDate' => $params['date']), 'setting');
            }
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
                drupal_add_js('https://25livepub.collegenet.com/scripts/spuds.js', array('type' => 'external', 'defer' => TRUE));
                drupal_add_js(array('stanfordR25Spud' => $rooms[$room]['spud_name']), 'setting');
                drupal_add_js(drupal_get_path('module', 'stanford_r25') . '/stanford_r25_spud.js');
            } else {
                // if the calendar is FullCalendar, output links to the required javascript files and css
                drupal_add_css(libraries_get_path('fullcalendar') . '/fullcalendar.css');
                drupal_add_js(array('stanfordR25Timezone' => date_default_timezone()), 'setting');
                drupal_add_js(libraries_get_path('fullcalendar') . '/lib/moment.min.js');
                drupal_add_js(libraries_get_path('fullcalendar') . '/fullcalendar.min.js');
                // if we have the qtip tooltip library, add that
                if (libraries_get_path('jquery.qtip') !== false && user_is_logged_in()) {
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
        } else {
            $output['no_room'] = array(
                '#type' => 'markup',
                '#markup' => '<span>The requested room is not currently available.</span>',
            );
        }
    } else {
        $output = MENU_NOT_FOUND;
    }
    return $output;
}

// the room info form callback is called by the room info block.
// it simply outputs room information that originally came from 25Live but is
// stored in the variables table in our database
//
// this code created the room info block as a form so we could have a fieldset popdown
// but it was requested to be removed, so now we can just output the block in its view hook
// in the main module
/*
function stanford_r25_room_info_form($form, &$form_state, $room_info = '')
{
        $form['stanford_r25_room_set'] = array(
            '#type' => 'fieldset',
            '#title' => t('View Room Info'),
            '#collapsible' => TRUE,
            '#collapsed' => TRUE,
        );
    $content = '<div id="stanford-r25-room-info">';
    if (!empty($room_info['photo_id'])) {
        $content .= '<div id="stanford-r25-room-photo"><img src="' . variable_get('stanford_r25_base_image_url', '') . '/' . $room_info['photo_id'] . '"></div>';
    }
    $content .= '<div id="stanford-r25-room-detail">';
    if (!empty($room_info['comments'])) {
        $content .= '<div id="stanford-r25-room-comments">' . $room_info['comments'] . '</div>';
    }
    if (!empty($room_info['layout_name'])) {
        $content .= '<div id="stanford-r25-room-layout"><b>Layout:</b> ' . $room_info['layout_name'] . '</div>';
    }
    if (!empty($room_info['layout_capacity'])) {
        $content .= '<div id="stanford-r25-room-capacity"><b>Capacity:</b> ' . $room_info['capacity'] . '</div>';
    }
    if (!empty($room_info['layout_features'])) {
        $content .= '<div id="stanford-r25-room-features"><b>Features:</b> ' . $room_info['layout_features'] . '</div>';
    }
    if (!empty($room_info['layout_categories'])) {
        $content .= '<div id="stanford-r25-room-categories"><b>Categories:</b> ' . $room_info['layout_categories'] . '</div>';
    }
    if (!empty($room_info['layout_instruction'])) {
        $content .= '<div id="stanford-r25-room-instruction"><b>Instructions:</b> ' . $room_info['layout_instruction'] . '</div>';
    }
    $content .= '</div></div>';
    $form['stanford_r25_room_set']['room_markup'] = array(  
    //$form['stanford_r25_room_markup'] = array(
        '#type' => 'markup',
        '#markup' => $content,
    );
    return $form;
}
*/

// the reservation form callback is called by the reservation block
function stanford_r25_reservation($form, &$form_state, $room = '', $max_headcount = 5, $contact_info = array())
{
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
        '#required' => true,
    );
    // default booking duration is limited to 2 hours in 30 minute increments
    // but a room can have a different value
    $max_hours = 2;
    if (!empty($rooms[$room]['max_hours'])) {
        $max_hours = intval($rooms[$room]['max_hours']);
        if ($max_hours == 0) $max_hours = 24;
    }
    $hours_array = array();
    if ($max_hours > 2) {
        for ($i = 0; $i < $max_hours; $i++) {
            $hstr = '';
            if ($i == 0) {
                $hours_array[] = '30 minutes';
                $hours_array[] = '1 hour';
            } else {
                $hours_array[] = strval($i) . '.5 hours';
                $hours_array[] = strval($i+1) . ' hours';
            }
        }
    } else {
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
        '#required' => true,
    );
    // max headcount for a room comes from parameter passed to the function
    $form['stanford_r25_booking_headcount'] = array(
        '#type' => 'select',
        '#title' => t('Headcount'),
        '#options' => array(),
        '#required' => true,
    );
    // add to the select list for the number of possible headcounts
    for ($i = 1; $i < $max_headcount + 1; $i++) {
        $form['stanford_r25_booking_headcount']['#options'][] = strval($i);
    }
    // every booking needs some reason text
    $form['stanford_r25_booking_reason'] = array(
        '#type' => 'textfield',
        '#title' => t('Reason'),
        '#required' => true,
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
                $form['stanford_r25_booking_attr'.$attr_id] = array(
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
                $form['stanford_r25_contact_'.$attr_id] = array(
                    '#type' => $field_type,
                    '#title' => $attr_info['name'],
                    '#default_value' => $contact_info,
                );
            }
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
    } else {
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
        '#markup' => check_markup($booking_instr['value'],$booking_instr['format']),
    );

    return $form;

}

/**
// some date and time formatting stuff - taking input from form date/time and duration fields
// and returning start and end times in W3C format to pass to the 25Live web services api.
function _stanford_r25_format_date_and_time($date, $booking_duration, $max_hours)
{
    //$date = DateTime::createFromFormat("Y-m-d g:i a", $booking_date['date'] . ' ' . $booking_date['time']);
    //$date_errors = DateTime::getLastErrors();
    // don't allow reservations more than 1/2 hour in the past. we're not a time machine.
    //if (date_timestamp_get($date) < (time() - 1800)) {
    //    $date = false;
    //}
    //if (!$date || $date_errors["warning_count"] > 0 || $date_errors["error_count"] > 0) {
    //    return false;
    // }
    $duration = intval($booking_duration);
    if ($duration < 0 || $duration > (($max_hours*2)-1)) return false;
    $duration = ($duration * 30) + 30;
    return array('day' => $date->format('Y-m-d'), 'start' => $date->format(DateTime::W3C),
        'end' => $date->add(new DateInterval('PT' . $duration . 'M'))->format(DateTime::W3C));
}
**/

// reservation form validation - formats the date strings and stores them in form 'storage' for the submit routine
function stanford_r25_reservation_validate($form, &$form_state)
{
    if (!user_access('book r25 rooms')) {
        form_set_error('stanford_r25_booking_reason', 'User does not have permission to book rooms.');
        return;
    }
    $booking_info = array();
    $rooms = _stanford_r25_room_config_load();
    if (empty($form_state['input']['stanford_r25_booking_roomid']) || !isset($rooms[$form_state['input']['stanford_r25_booking_roomid']])) {
        form_set_error('stanford_r25_booking_roomid', 'Invalid room id requested.');
        return;
    } else {
        $booking_info['room'] = $rooms[$form_state['input']['stanford_r25_booking_roomid']];
    }

    // make sure we have a valid date
    $booking_date = $form_state['input']['stanford_r25_booking_date'];
    $date = DateTime::createFromFormat("Y-m-d g:i a", $booking_date['date'] . ' ' . $booking_date['time']);
    $date_errors = DateTime::getLastErrors();
    if (!$date || $date_errors["warning_count"] > 0 || $date_errors["error_count"] > 0) {
        form_set_error('stanford_r25_booking_date','An invalid date was supplied.');
        return;
    }
    // don't allow reservations more than 1/2 hour in the past. we're not a time machine.
    if (date_timestamp_get($date) < (time() - 1800)) {
        form_set_error('stanford_r25_booking_date','A reservation in the past was requested. This isn\'t a time machine!');
        return;
    }

    // make sure date isn't blacked out if room checks for that
    if ($rooms[$form_state['input']['stanford_r25_booking_roomid']]['honor_blackouts'] == 1) {
        if (_stanford_r25_date_blacked_out(date_timestamp_get($date))) {
            form_set_error('stanford_r25_booking_date','This room is unavailable for reservation on the requested date. ' .
                'The room may only be reserved until the end of the current quarter. Please see your department ' .
                'administrator for more information.');
            return;
        }
    }

    // build 25Live date strings
    $duration = intval($form_state['input']['stanford_r25_booking_duration']);
    if ($duration < 0 || $duration > (($booking_info['room']['max_hours']*2)-1)) {
        form_set_error('stanford_r25_booking_duration','Invalid duration was submitted.');
        return;
    }
    $duration = ($duration * 30) + 30;
    $date_strs =  array('day' => $date->format('Y-m-d'), 'start' => $date->format(DateTime::W3C),
        'end' => $date->add(new DateInterval('PT' . $duration . 'M'))->format(DateTime::W3C));

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
function stanford_r25_reservation_submit($form, &$form_state)
{
    // make sure the user has access, that the needed information is available, and the room is bookable
    if (!user_access('book r25 rooms')) {
        drupal_set_message('You do not have permission to book this room.', 'error');
        unset($form_state['storage']['stanford_r25_booking']);
        return;
    }
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
    
    $secgroup_id = intval($form_state['storage']['stanford_r25_booking']['room']['approver_secgroup_id']);
    $mail_list = '';
    $todo_insert = '';
    if ($event_state == STANFORD_R25_ROOM_STATUS_TENTATIVE && $secgroup_id > 0) {
        $todo_str = file_get_contents(drupal_get_path('module', 'stanford_r25') . '/stanford_r25_reserve_todo.xml');
        $result = _stanford_r25_api_call('r25users', null, null, $secgroup_id);
        if (!empty($result['index']['R25:CONTACT_ID'])) {
            foreach ($result['index']['R25:CONTACT_ID'] as $key => $value) {
                if (!empty($result['vals'][$value]['value'])) {
                    $todo_temp = str_replace('[r25_start_date_time]', $form_state['storage']['stanford_r25_booking']['dates']['start'],$todo_str);
                    $todo_temp = str_replace('[r25_approver_id]', $result['vals'][$value]['value'],$todo_temp);
                    $todo_temp = str_replace('[r25_credential_id]', variable_get('stanford_r25_credential_contact_id', ''), $todo_temp);
                    $todo_insert .= $todo_temp;
                }
            }
        }
        if (!empty($result['index']['R25:PRIMARY_EMAIL'])) {
            foreach ($result['index']['R25:PRIMARY_EMAIL'] as $key => $value) {
                if (!empty($result['vals'][$value]['value'])) {
                    if (!empty($mail_list)) $mail_list .= ', ';
                    $mail_list .= $result['vals'][$value]['value'];
                }
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
    $xml = str_replace('[r25_todo]',$todo_insert, $xml);
    global $user;
    $xml = str_replace('[r25_created_by]', '<span>Self service reservation made by ' . $user->name . ' - <a href="mailto:' . $user->mail . '">click to contact by email.</a></span>', $xml);

    // send the request to our api function
    $result = _stanford_r25_api_call('reserve', null, null, $xml);

    // check the results to see if our reservation attempt was successful
    $success = false;
    if ($result) {
        // a successful return with no status message is assumed to be a success since that's how the webservices api works. go figure.
        // if we use the setting that returns a positive return code for success, then other information is missing.
        if (empty($result['index']['R25:MSG_ID'][0])) {
            $success = true;
        } else {
            // even though we should not see a success code, we do want to check if we got a failure code, which is anything but the
            // two defined success codes.
            $msg_index = $result['index']['R25:MSG_ID'][0];
            if (!empty($result['vals'][$msg_index]['value'])) {
                if ($result['vals'][$msg_index]['value'] === 'EV_I_SAVE' ||
                    $result['vals'][$msg_index]['value'] === 'EV_I_CREATED'
                ) {
                    $success = true;
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

        // send an email about the booking if mail list is set
        if (!empty($mail_list) && !empty($form_state['storage']['stanford_r25_booking']['room']['email_list'])) $mail_list .= ', ';
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
        } else if ($state == 2) {
            // this is the email for a confirmed booking
            $subject = 'Room Reservation';
            $body[0] .= '.';
            $body[] = 'View the reservation at: https://25live.collegenet.com/stanford/#details&obj_type=event&obj_id=' . $result['vals'][$result['index']['R25:EVENT_ID'][0]]['value'];
        }
        $body[] = "Room: " . $form_state['storage']['stanford_r25_booking']['room']['display_name'];
        $body[] = "Date: " . $date->format("l, F j, Y g:i a");
        $duration = (intval($form_state['values']['stanford_r25_booking_duration']) * 30) + 30;
        if ($duration > 120) {
            $body[] = 'Duration: ' . $duration/60 . ' hours';
        } else {
            $body[] = 'Duration: ' . $duration . ' minutes';
        }
        $body[] = "Reason: " . $form_state['values']['stanford_r25_booking_reason'];
        $body[] = "Requested by: " . $user->name . " " . $user->mail;
        $params = array('body' => $body,
            'subject' => $subject
        );
        drupal_mail('stanford_r25', $result['vals'][$result['index']['R25:EVENT_ID'][0]]['value'], $mail_list, language_default(), $params);

    } else {
        // display a message if the booking failed
        drupal_set_message('The system was unable to book your room. This may be because of a time conflict with another meeting, or because someone else booked it first or because of problems communicating with 25Live. Please try again.', 'error');
        $body = array();
        $event_id = 'unknown';
        if (!empty($result['index']['R25:EVENT_ID'][0]) && !empty($result['vals'][$result['index']['R25:EVENT_ID'][0]]['value'])) {
            $event_id = $result['vals'][$result['index']['R25:EVENT_ID'][0]]['value'];
            $body[] = 'failed reservation at: https://25live.collegenet.com/stanford/#details&obj_type=event&obj_id=' . $result['vals'][$result['index']['R25:EVENT_ID'][0]]['value'];
        }
        $xyz = print_r($result,TRUE);
        $body[] = 'failed reservation. no event id. results:<pre>'.$xyz.'</pre>';
        drupal_mail('stanford_r25', $event_id, 'ksharp@stanford.edu', language_default(), array('body'=>$body, 'subject'=>'r25 reservation failure'));
//        _stanford_r25_api_call('delete', null, null, $result['vals'][$result['index']['R25:EVENT_ID'][0]]['value']);
    }
    // remove the storage we set up in validate
    unset($form_state['storage']['stanford_r25_booking']);
}
