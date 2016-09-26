<?php

// takes ajax requests from FullCalendar, passes them on to the 25Live web api, and returns the results as JSON

// bit of DRY code to return values from the web services api xml
function _stanford_r25_feed_get_value(&$results, $name, $key)
{
    $return_val = '';
    if (isset($results['vals'][$results['index'][$name][$key]]['value'])) {
        $return_val = $results['vals'][$results['index'][$name][$key]]['value'];
    }
    return $return_val;
}

// This is the ajax function called by FullCalendar with a room id, start date and end date in the POST data.
// Returns JSON array of event data to FullCalendar
function _stanford_r25_feed()
{

    // format the request to the 25Live API
    $rooms = _stanford_r25_room_config_load();
    $space_id = null;
    if (!empty($_POST['room_id']) && !empty($rooms[$_POST['room_id']]['space_id'])) {
        $space_id = $rooms[$_POST['room_id']]['space_id'];
    } else {
        drupal_not_found();
    }
    $start = str_replace('-', '', $_POST['start']);
    $end = str_replace('-', '', $_POST['end']);
    $args = 'space_id=' . $space_id . '&scope=extended&start_dt=' . $start . '&end_dt=' . $end;
    $items = array();
    // make the API call
    $results = _stanford_r25_api_call('feed', null, null, $args);
    if ($results && !empty($results['index']['R25:RESERVATION_ID'])) {
        // for each result, store the data in the return array
        foreach ($results['index']['R25:RESERVATION_ID'] as $key => $value) {
            $id = $results['vals'][$value]['value'];
            $title = _stanford_r25_feed_get_value($results, 'R25:EVENT_NAME', $key);
            if ($space_id == '395') $title = drupal_html_to_text(_stanford_r25_feed_get_value($results, 'R25:TEXT', $key));
            $start = _stanford_r25_feed_get_value($results, 'R25:RESERVATION_START_DT', $key);
            $end = _stanford_r25_feed_get_value($results, 'R25:RESERVATION_END_DT', $key);
            $headcount = _stanford_r25_feed_get_value($results, 'R25:EXPECTED_COUNT', $key);
            $state = _stanford_r25_feed_get_value($results, 'R25:STATE', $key);
            $state_text = _stanford_r25_feed_get_value($results, 'R25:STATE_NAME', $key);
            $items[] = array(
                'id' => $id,
                'title' => $title,
                'start' => $start,
                'end' => $end,
                'headcount' => $headcount,
                'state' => $state,
                'state_name' => $state_text,
                'tip' => '',
            );
        }

        // for logged in users, we want to display event status, headcount, and who did the booking
        if (user_is_logged_in()) {
            foreach ($results['index']['R25:TEXT_TYPE_ID'] as $key => $value) {
                if ($results['vals'][$value]['value'] === "1") {
                    $text = '';
                    if (isset($results['index']['R25:TEXT'][$key])) {
                        $text = $results['vals'][$results['index']['R25:TEXT'][$key]]['value'];
                    }
                    if (!empty($text)) {
                        $index = count($results['index']['R25:RESERVATION_ID']);
                        while ($index) {
                            $index -= 1;
                            if (intval($value) > intval($results['index']['R25:RESERVATION_ID'][$index])) {
                                $items[$index]['description'] = $text;
                                break;
                            }
                        }
                    }
                }
            }
            foreach ($items as $key => $item) {
                if (intval($item['state']) == 1) {
                    $items[$key]['backgroundColor'] = 'goldenrod';
                    $items[$key]['textColor'] = 'black';
                }
                $items[$key]['tip'] = 'Status: ' . $item['state_name'] . '<br />' . 'Headcount: ' . $item['headcount'];
                if (!empty($item['description'])) {
                    $items[$key]['tip'] .= '<br />' . $item['description'];
                }
            }
        }
    }
    drupal_json_output($items);
}
