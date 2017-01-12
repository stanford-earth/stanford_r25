<?php

// takes ajax requests from FullCalendar, passes them on to the 25Live web api, and returns the results as JSON

// bit of DRY code to return values from the web services api xml
function _stanford_r25_feed_get_value(&$results, $name, $key) {
  $return_val = '';
  if (isset($results['vals'][$results['index'][$name][$key]]['value'])) {
    $return_val = $results['vals'][$results['index'][$name][$key]]['value'];
  }
  return $return_val;
}

// This is the ajax function called by FullCalendar with a room id, start date and end date in the POST data.
// Returns JSON array of event data to FullCalendar
function _stanford_r25_feed($get_room_id = NULL) {

  // format the request to the 25Live API from either POST or GET arrays
  $rooms = _stanford_r25_room_config_load();
  $room_id = '';
  $space_id = NULL;
  $start = '';
  $end = '';
  if (!empty($_POST['room_id']) && !empty($rooms[$_POST['room_id']]['space_id'])) {
    $room_id = $_POST['room_id'];
    $space_id = $rooms[$_POST['room_id']]['space_id'];
    $start = str_replace('-', '', $_POST['start']);
    $end = str_replace('-', '', $_POST['end']);
  }
  else {
    if (empty($get_room_id) || empty($rooms[$get_room_id]['space_id'])) {
      drupal_not_found();
    }
    else {
      $room_id = $get_room_id;
      $space_id = $rooms[$get_room_id]['space_id'];
      if (!empty($_GET['start'])) {
        $start = str_replace('-', '', $_GET['start']);
      }
      if (!empty($_GET['end'])) {
        $end = str_replace('-', '', $_GET['end']);
      }
    }
  }
  
  // depending on the logged in user requesting this information, we want to include
  // links to contact the event scheduler, or to confirm or cancel the event
  $approver_list = array();
  if (!empty($rooms[$room_id]['approver_secgroup_id'])) {
    $approver_list = _stanford_r25_security_group_emails($rooms[$room_id]['approver_secgroup_id']);
  }
  $approver = FALSE;
  
  // if the user is Drupal user 1 or can administer rooms, let them approve
  // and cancel events
  global $user;
  if ($user->uid == 1 || user_access('administer stanford r25') ||
    ($user->uid > 0 && in_array($user->mail, $approver_list))
  ) {
    $approver = TRUE;
  }
  
  // build the 25Live API request with the space id for the requested room and 
  // for the start and end dates requested by fullcalendar
  $args = 'space_id=' . $space_id . '&scope=extended&start_dt=' . $start . '&end_dt=' . $end;
  $items = array();
  // make the API call
  $results = _stanford_r25_api_call('feed', NULL, NULL, $args);
  if ($results && !empty($results['index']['R25:RESERVATION_ID'])) {
    // for each result, store the data in the return array
    foreach ($results['index']['R25:RESERVATION_ID'] as $key => $value) {
      $id = $results['vals'][$value]['value'];
      $event_id = _stanford_r25_feed_get_value($results, 'R25:EVENT_ID', $key);
      $title = _stanford_r25_feed_get_value($results, 'R25:EVENT_NAME', $key);
      $start = _stanford_r25_feed_get_value($results, 'R25:RESERVATION_START_DT', $key);
      $end = _stanford_r25_feed_get_value($results, 'R25:RESERVATION_END_DT', $key);
      $headcount = _stanford_r25_feed_get_value($results, 'R25:EXPECTED_COUNT', $key);
      $state = _stanford_r25_feed_get_value($results, 'R25:STATE', $key);
      $state_text = _stanford_r25_feed_get_value($results, 'R25:STATE_NAME', $key);
      $items[] = array(
        'id' => $id,
        'event_id' => $event_id,
        'index' => $value,
        'title' => $title,
        'start' => $start,
        'end' => $end,
        'headcount' => $headcount,
        'state' => $state,
        'state_name' => $state_text,
        'scheduled_by' => '',
        'tip' => '',
      );
    }

    // for logged in users, we want to display event status, headcount, and who did the booking
    if (user_is_logged_in()) {
      // find out if event was *not* scheduled by QuickBook account and then get the scheduler
      $quickbook_id = intval(variable_get('stanford_r25_credential_contact_id', '0'));
      foreach ($results['index']['R25:SCHEDULER_ID'] as $key => $value) {
        if (intval($results['vals'][$value]['value']) != $quickbook_id) {
          $text = '';
          if (isset($results['vals'][$results['index']['R25:SCHEDULER_ID'][$key]]['value'])) {
            $text = 'Reservation scheduled in 25Live by ' .
              $results['vals'][$results['index']['R25:SCHEDULER_NAME'][$key]]['value'] . '.';
          }
          if (isset($results['vals'][$results['index']['R25:SCHEDULER_EMAIL'][$key]]['value'])) {
            $email = $results['vals'][$results['index']['R25:SCHEDULER_EMAIL'][$key]]['value'];
            if (empty($text)) {
              $text = 'Reservation scheduled in 25Live by ' . $email . '.';
            }
            $text .= '&nbsp;<a href="mailto:' . $email . '">Click to contact scheduler by email</a>.';
          }
          if (empty($text)) {
            $text = 'Scheduler is unknown.';
          }
          $index = count($items);
          while ($index) {
            $index -= 1;
            if (intval($value) > intval($items[$index]['index'])) {
              $items[$index]['scheduled_by'] = $text;
              break;
            }
          }
        }
      }

      // for those items that were scheduled by quickbook, the event description contains the scheduler.
      // also, certain rooms may want to show the description as the FullCalendar event title
      foreach ($results['index']['R25:EVENT_DESCRIPTION'] as $key => $value) {
        $text = '';
        if (!empty($results['vals'][$results['index']['R25:EVENT_DESCRIPTION'][$key]]['value'])) {
          $text = $results['vals'][$results['index']['R25:EVENT_DESCRIPTION'][$key]]['value'];
        }
        if (!empty($text)) {
          $index = count($items);
          while ($index) {
            $index -= 1;
            if (intval($value) > intval($items[$index]['index'])) {
              // display event description as event title if room is so marked
              if (isset($rooms[$room_id]['description_as_title']) &&
                intval($rooms[$room_id]['description_as_title']) == 1
              ) {
                $items[$index]['title'] = drupal_html_to_text($text);
              }
              $items[$index]['description'] = $text;
              break;
            }
          }
        }
      }
      foreach ($items as $key => $item) {
        $can_confirm = FALSE;
        if (intval($item['state']) == 1) {
          $items[$key]['backgroundColor'] = 'goldenrod';
          $items[$key]['textColor'] = 'black';
          if ($approver) {
            $can_confirm = TRUE;
          }
        }
        $items[$key]['tip'] = 'Status: ' . $item['state_name'] . '<br />' . 'Headcount: ' . $item['headcount'];
        if (!empty($item['description'])) {
          $items[$key]['tip'] .= '<br />' . $item['description'];
        }
        if (!empty($item['scheduled_by'])) {
          $items[$key]['tip'] .= '<br />' . $item['scheduled_by'];
        }
        drupal_alter('stanford_r25_contact', $items[$key]['tip']);
        $can_cancel = FALSE;
        if ($approver) {
          $can_cancel = TRUE;
        }
        else {
          $scheduler_email = '';
          $description = $items[$key]['tip'];
          $mailto_pos = strpos($description, '"mailto:');
          if ($mailto_pos !== FALSE) {
            $mailto_endpos = strpos($description, '"', $mailto_pos + 8);
            if ($mailto_endpos !== FALSE) {
              $scheduler_email = substr($description, $mailto_pos + 8, $mailto_endpos - ($mailto_pos + 8));
            }
          }
          if (!empty($scheduler_email) && $scheduler_email === $user->mail) {
            $can_cancel = TRUE;
          }
        }

        if ($can_confirm) {
          $items[$key]['tip'] .= '<br /><a href="' . url('/r25/' . $_POST['room_id'] . '/confirm_reservation/' . $items[$key]['event_id'] . '/' .
              $items[$key]['start']) . '">Click to confirm reservation</a>';
        }
        if ($can_cancel) {
          $items[$key]['tip'] .= '<br /><a href="' . url('/r25/' . $_POST['room_id'] . '/cancel_reservation/' . $items[$key]['event_id'] . '/' .
              $items[$key]['start']) . '">Click to cancel reservation</a>';
        }
        if ($approver) {
          $items[$key]['tip'] .= '<br /><a href="https://25live.collegenet.com/stanford/#details&obj_type=event&obj_id=' . $items[$key]['event_id'] . 
            '">Click to manage in 25Live</a>';
        }
      }
    }
  }
  drupal_json_output($items);  // return a json version of the output array to full calendar
}
