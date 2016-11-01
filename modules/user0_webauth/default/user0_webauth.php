<?php
    if (!empty($_REQUEST['base_path']) && !empty($_REQUEST['cookie_domain']) &&
        !empty($_REQUEST['destination'])) {
        $tmp_dir = substr($_SERVER['SCRIPT_FILENAME'],0,strpos($_SERVER['SCRIPT_FILENAME'],'user0_webauth.php')) . 'tmp';
        $tempnam = tempnam($tmp_dir,'');
        $ldap_vars = array();
        foreach ($_SERVER as $key => $value) {
            if (substr_compare($key, 'WEBAUTH_LDAP_',0,13) == 0) {
                $ldap_vars[$key] = $value;
            }
        }
        $ldap_vars['WEBAUTH_LDAP_DESTINATION'] = $_REQUEST['destination'];
        if (file_put_contents($tempnam,json_encode($ldap_vars)) !== false) {
            $fname = substr($tempnam,strrpos($tempnam,'/')+1);
            setcookie('user0_webauth',$fname,0,$_REQUEST['base_path'],$_REQUEST['cookie_domain'],TRUE,TRUE);
        }
    	header('Location: https://'.$_SERVER['HTTP_HOST'].$_REQUEST['base_path'].$_REQUEST['destination']);
    } else {
	header('HTTP/1.0 404 Not Found');
        header('Location: https://'.$_SERVER['HTTP_HOST']);
    }
?>
