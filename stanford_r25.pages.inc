<?php

function _stanford_r25_calendar_page($room = null)
{
    $rooms = _stanford_r25_room_config_load();
    $output = array();
    if (!empty($room) && isset($rooms[$room]['status'])) {
        $status = intval($rooms[$room]['status']);
        if ($status > STANFORD_R25_ROOM_STATUS_DISABLED) {
            drupal_add_js(array('stanfordR25Room' => $room), 'setting');
            drupal_add_js(array('stanfordR25Status' => $status), 'setting');
            $can_book =  (user_access('book r25 rooms') ? 1 : 0);
            drupal_add_js(array('stanfordR25Access' => $can_book), 'setting');
            if (intval($rooms[$room]['caltype']) == 1) {
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
                drupal_add_css(libraries_get_path('fullcalendar') . '/fullcalendar.css');
                drupal_add_js(array('stanfordR25Timezone' => date_default_timezone()), 'setting');
                drupal_add_js(libraries_get_path('fullcalendar') . '/lib/moment.min.js');
                drupal_add_js(libraries_get_path('fullcalendar') . '/fullcalendar.min.js');
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
function _stanford_r25_room_info_form($form, &$form_state, $room_info = '') {
    $form['stanford_r25_room_set'] = array(
        '#type' => 'fieldset',
        '#title' => t('View Room Info'),
        '#collapsible' => TRUE,
        '#collapsed' => TRUE,
    );
    $content = '<div id="stanford-r25-room-info">';
    if (!empty($room_info['photo_id'])) {
        $content .= '<div id="stanford-r25-room-photo"><img src="'.variable_get('stanford_r25_base_image_url','') . '/' . $room_info['photo_id'] . '"></div>';
    }
    $content .= '<div id="stanford-r25-room-detail">';
    if (!empty($room_info['comments'])) {
        $content .= '<div id="stanford-r25-room-comments">' . $room_info['comments'] . '</div>';
    }
    if (!empty($room_info['layout_name'])) {
        $content .= '<div id="stanford-r25-room-layout"><b>Layout:</b> ' . $room_info['layout_name'] . '</div>';
    }
    if (!empty($room_info['layout_capacity'])) {
        $content .= '<div id="stanford-r25-room-capacity"><b>Capacity:</b> ' . $room_info['layout_capacity'] . ' ' . STANFORD_R25_MENU_PATH .  '</div>';
    }
    if (!empty($room_info['layout_features'])) {
        $content .= '<div id="stanford-r25-room-features"><b>Features:</b> ' . $room_info['layout_features']. '</div>';
    }
    if (!empty($room_info['layout_categories'])) {
        $content .= '<div id="stanford-r25-room-categories"><b>Categories:</b> ' . $room_info['layout_categories'] . '</div>';
    }
    if (!empty($room_info['layout_instruction'])) {
        $content .= '<div id="stanford-r25-room-instruction"><b>Instructions:</b> '.$room_info['layout_instruction'] . '</div>';
    }
    $content .= '</div></div>';
    $form['stanford_r25_room_set']['room_markup'] = array(
        '#type'=> 'markup',
        '#markup' => $content,
    );
    return $form;
}

function _stanford_r25_reservation($form, &$form_state, $room = '', $max_headcount = 5)
{

    $form['stanford_r25_booking_roomid'] = array(
        '#type' => 'hidden',
        '#value' => $room,
    );
    $rooms = _stanford_r25_room_config_load();
    if (!empty($rooms[$room]['status']) && intval($rooms[$room]['status']) == STANFORD_R25_ROOM_STATUS_TENTATIVE) {
        $form['stanford_r25_booking_tentative'] = array(
            '#type'=>'markup',
            '#markup'=>"<p>This room only accepts tentative reservations which must be approved by the room's administrator.</p>",
        );
    }
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
    $form['stanford_r25_booking_duration'] = array(
        '#type' => 'select',
        '#title' => t('Duration'),
        '#options' => array(
            0 => t('30 minutes'),
            1 => t('60 minutes'),
            2 => t('90 minutes'),
            3 => t('120 minutes'),
        ),
        '#default_value' => 0,
        '#required' => true,
    );
    $form['stanford_r25_booking_headcount'] = array(
        '#type' => 'select',
        '#title' => t('Headcount'),
        '#options' => array(),
        //'#default_value' => 0,
        '#required' => true,
    );
    for ($i = 1; $i < $max_headcount + 1; $i++) {
        $form['stanford_r25_booking_headcount']['#options'][] = strval($i);
    }
    //$form['stanford_r25_booking_headcount']['#options'][] = '> 10';
    $form['stanford_r25_booking_reason'] = array(
        '#type' => 'textfield',
        '#title' => t('Reason'),
        '#required' => true,
    );

    $form['submit'] = array(
        '#type' => 'submit',
        '#value' => t('Reserve'),
    );
    $form['r25_instructions'] = array(
        '#type' => 'markup',
        '#markup' => check_markup(variable_get('stanford_r25_booking_instructions', '')),
    );

    return $form;

}

function _stanford_r25_format_date_and_time($booking_date, $booking_duration)
{
    //$date = DateTime::createFromFormat("D, M d, Y g:i a", $booking_date.' '.$booking_time);
    $date = DateTime::createFromFormat("Y-m-d g:i a", $booking_date['date'] . ' ' . $booking_date['time']);
    $date_errors = DateTime::getLastErrors();
    if (!$date || $date_errors["warning_count"] > 0 || $date_errors["error_count"] > 0) {
        return false;
    }
    $duration = intval($booking_duration);
    if ($duration < 0 || $duration > 3) return false;
    $duration = ($duration * 30) + 30;
    return array('day' => $date->format('Y-m-d'), 'start' => $date->format(DateTime::W3C),
        'end' => $date->add(new DateInterval('PT' . $duration . 'M'))->format(DateTime::W3C));
}

function _stanford_r25_reservation_validate($form, &$form_state)
{
    if (!user_access('book r25 rooms')) {
        form_set_error('stanford_r25_booking_reason', 'User does not have permission to book rooms.');
        return;
    }
    $booking_info = array();
    $rooms = _stanford_r25_room_config_load();
    if (empty($form_state['input']['stanford_r25_booking_roomid']) || !isset($rooms[$form_state['input']['stanford_r25_booking_roomid']])) {
        form_set_error('stanford_r25_booking_roomid', 'Invalid room id requested.');
    } else {
        $booking_info['room'] = $rooms[$form_state['input']['stanford_r25_booking_roomid']];
    }
    $date_strs = _stanford_r25_format_date_and_time($form_state['input']['stanford_r25_booking_date'],
        $form_state['input']['stanford_r25_booking_duration']);
    if ($date_strs === false) {
        form_set_error('stanford_r25_booking_time', 'Invalid date and time or duration submitted.');
    } else {
        $booking_info['dates'] = $date_strs;
    }
    if (!empty($booking_info)) $form_state['storage']['stanford_r25_booking'] = $booking_info;
}

function _stanford_r25_reservation_submit($form, &$form_state)
{
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
    $event_state = $event_state - 1;
    $xml = file_get_contents(drupal_get_path('module', 'stanford_r25') . '/stanford_r25_reserve.xml');
    $xml = str_replace('[r25_event_name]', $form_state['values']['stanford_r25_booking_reason'], $xml);
    $xml = str_replace('[r25_parent_id]', variable_get('stanford_r25_parent_event_id', 'unknown'), $xml);
    $xml = str_replace('[r25_event_type]', variable_get('stanford_r25_event_type', 'unknown'), $xml);
    $xml = str_replace('[r25_event_state]', $event_state, $xml);
    $xml = str_replace('[r25_organization_id]', variable_get('stanford_r25_org_id', 'unknown'), $xml);
    $xml = str_replace('[r25_expected_headcount]', $form_state['complete form']['stanford_r25_booking_headcount']['#options'][$form_state['values']['stanford_r25_booking_headcount']], $xml);
    $xml = str_replace('[r25_start_date_time]', $form_state['storage']['stanford_r25_booking']['dates']['start'], $xml);
    $xml = str_replace('[r25_end_date_time]', $form_state['storage']['stanford_r25_booking']['dates']['end'], $xml);
    $xml = str_replace('[r25_space_id]', $form_state['storage']['stanford_r25_booking']['room']['space_id'], $xml);
    global $user;
    $xml = str_replace('[r25_created_by]', '<p>Self service reservation made by ' . $user->name . ' - <a href="mailto:' . $user->mail . '">click to contact</a></p>', $xml);
    $result = _stanford_r25_api_call('reserve', null, null, $xml);
    $success = false;
    if ($result) {
        if (empty($result['index']['R25:MSG_ID'][0])) {
            $success = true;
        } else {
            $msg_index = $result['index']['R25:MSG_ID'][0];
            if (!empty($result['vals'][$msg_index]['value'])) {
                if ($result['vals'][$msg_index]['value'] === 'EV_I_SAVE') {
                    $success = true;
                }
            }
        }
    }
    if ($success) {
        $date = DateTime::createFromFormat(DATE_W3C, $form_state['storage']['stanford_r25_booking']['dates']['start']);
        $state = intval($result['vals'][$result['index']['R25:STATE'][0]]['value']);
        $msg = $form_state['storage']['stanford_r25_booking']['room']['display_name'] . ' has a <b>' . $result['vals'][$result['index']['R25:STATE_NAME'][0]]['value'] . '</b> reservation for "' . $form_state['values']['stanford_r25_booking_reason'] . '" on  ' . $date->format("l, F j, Y g:i a") . '.';
        if (intval($result['vals'][$result['index']['R25:STATE'][0]]['value']) == 1) {
            $msg .= ' The room administrator will confirm or deny your request.';
        }
        drupal_set_message($msg);

        // send an email about the booking if mail list is set
        $mail_list = $form_state['storage']['stanford_r25_booking']['room']['email_list'];
        $body = array();
        $body[] = "A " . $result['vals'][$result['index']['R25:STATE_NAME'][0]]['value'] . "reservation has been made";
        $subject = '';
        if ($state == 1) {
            $subject = 'Room Reservation Request - ACTION REQUIRED';
            $body[0] .= ' requiring your approval.';
            $body[] = 'You may view this request in 25Live and confirm or deny it at this link (requires you first be logged in to 25Live): ';
            $body[] = 'https://25live.collegenet.com/stanford/#details&obj_type=event&obj_id='.$result['vals'][$result['index']['R25:EVENT_ID'][0]]['value'];
            $body[] = '';
        } else if ($state == 2) {
            $subject = 'Room Reservation';
            $body[0] .= '.';
            $body[] = 'View the reservation at: https://25live.collegenet.com/stanford/#details&obj_type=event&obj_id='.$result['vals'][$result['index']['R25:EVENT_ID'][0]]['value'];
        }
        $body[] = "Room: " . $form_state['storage']['stanford_r25_booking']['room']['display_name'];
        $body[] = "Date: " . $date->format("l, F j, Y g:i a");
        $duration = (intval($form_state['values']['stanford_r25_booking_duration'])*30)+30;
        $body[] = "Duration: " . $duration . ' minutes';
        $body[] = "Reason: " . $form_state['values']['stanford_r25_booking_reason'];
        $body[] = "Requested by: " . $user->name . " " . $user->mail;
        $params = array('body'=>$body,
            'subject'=>$subject
        );
        drupal_mail('stanford_r25',$result['vals'][$result['index']['R25:EVENT_ID'][0]]['value'],$mail_list,language_default(),$params);

    } else {
        drupal_set_message('The system was unable to book your room. This may be because of a time conflict with another meeting, or because someone else booked it first or because of problems communicating with 25Live. Please try again.', 'error');
        _stanford_r25_api_call('delete', null, null, $result['vals'][$result['index']['R25:EVENT_ID'][0]]['value']);
    }
    unset($form_state['storage']['stanford_r25_booking']);
}
