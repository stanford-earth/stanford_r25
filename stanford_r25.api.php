<?php

// examples of calls to stanford_r25 hooks

/****
 *  Allow a module to alter the string displayed on the tooltip created by
 *  the calendar feed
 *
 *  @param string $contact_string
 */
function hook_stanford_r25_contact_alter(&$contact_string) {
  $contact_string = 'No contact allowed!';
}

/****
 *  Allow a module to alter the calendar limit displayed by fullcalendar
 *  and the reservation form.
 *
 *  @param array $calendar_limit
 * 
 */
function hook_stanford_r25_fullcalendar_limit_alter(&$calendar_limit) {
  if (!empty($calendar_limit['room']) && substr($calendar_limit['room'],0,3) == 'mh0') {
    if (user_access('administer stanford r25')) {
      $calendar_limit['year'] = date('Y')+3;
    } else {
      $month = date('n');
      $year = date('Y');
      $calendar_limit['month'] = 12;
      if ($month < 10) {
        $calendar_limit['year'] = $year;
      } else {
        $calendar_limit['year'] = $year + 1;
      }
    }
  }
}

/****
 *  Allow a module to change the billable status of a room from true to false;
 *  used by a Stanford Earth custom module to make the Hartley room, normally
 *  billable, free to Stanford Earth associates.
 *
 *  @param boolean $is_billable
 */
function hook_stanford_r25_isbillable_alter(&$is_billable) {
  if ($is_billable) {
    $se3_user = _se3_hartley_is_se3_user();
    if ($se3_user) {
      $is_billable = false;
    }
  }
}

/****
 *  Provide a Drupal link to a page that will authenticate a non-Drupal user
 *  without creating a Drupal account for them
 *
 *  @param string $link_text
 *   The text to present to the user, for example 'Click here to Login'
 *   Defaults to 'Authenticate'
 *
 *  @return string
 *    Return a Drupal-formatted hyperlink for the authentication link
 */
function hook_stanford_r25_external_link($link_text = 'Authenticate') {

  global $base_url, $base_path, $cookie_domain;
  // make sure we're using https
  $secure_base_url = str_replace('http:', 'https:', $base_url) . '/';
  // return a link, in this case to
  // https://<drupal_base>/sites/default/user0_webauth/user0_webauth.php
  // as defined in the user0_webauth submodule included with this module
  $user0_webauth_url = $secure_base_url . conf_path() .
    '/user0_webauth/user0_webauth.php';
  // tell the authenticator where to return and use a class button
  $destination = drupal_get_destination();
  $options = array('attributes'=>array('class'=>array('button')),
    'query'=>array(
      'base_path' => $base_path,
      'cookie_domain' => $cookie_domain,
      'destination' => $destination['destination']));
  // return the formatted link
  return l($link_text, $user0_webauth_url, $options);
}

/****
 * Provide an array of information about an externally authenticated user
 *
 * @return array
 *   of information about the externally authenticated user with, in this
 *   example, keys such as R25_EXTERNAL_UID, R25_EXTERNAL_DISPLAYNAME,
 *   R25_EXTERNAL_MAIL, R25_EXTERNAL_POSTALADDRESS, etc. In this example,
 *   the authenticated user data is stored in a JSON temp file identified
 *   by a cookie.
 *
 */
function hook_stanford_r25_external_user() {
  $return_data = false;
  if (!empty($_COOKIE['user0_webauth'])) {
    $filename = _user0_webauth_tmp_directory() . '/' . $_COOKIE['user0_webauth'];
    $user0_info = array();
    if (file_exists($filename) && time()-filemtime($filename) < 36000) {
      $user0_info = json_decode(file_get_contents($filename),true);
    }
    if (empty($user0_info) || empty($user0_info['R25_EXTERNAL_UID'])) {
      watchdog('stanford_r25','External login failed. Missing data file or uid.');
      @unlink($filename);
    } else {
      $return_data = $user0_info;
    }
  }
  return $return_data;
}

/****
 * Provide a contact display string for an externally authenticated user
 *
 * @param array $user_info
 *   the user array created by hook_stanford_r25_external_user
 *
 * @return array
 *   a string formatted from information in the array
 *
 */
function hook_stanford_r25_external_user_display($user_info) {
  $contact = '';
  if (!is_array($user_info)) return $contact;
  if (!empty($user_info['R25_EXTERNAL_DISPLAYNAME'])) $contact .= $user_info['R25_EXTERNAL_DISPLAYNAME']."\r\n";
  if (!empty($user_info['R25_EXTERNAL_OU'])) $contact .= $user_info['R25_EXTERNAL_OU'] . "\r\n";
  if (!empty($user_info['R25_EXTERNAL_MAIL'])) $contact .= 'Mail: '.$user_info['R25_EXTERNAL_MAIL'] . "\r\n";
  if (!empty($user_info['R25_EXTERNAL_TELEPHONENUMBER'])) $contact .= 'Phone: '.$user_info['R25_EXTERNAL_TELEPHONENUMBER']."\r\n";
  if (!empty($user_info['R25_EXTERNAL_POSTALADDRESS'])) $contact .= 'Address: '.$user_info['R25_EXTERNAL_POSTALADDRESS'] . "\r\n";
  return $contact;
}
