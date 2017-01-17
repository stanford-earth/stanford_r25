<?php
    // this script, running under webauth, creates a temp file with LDAP information
    // about the logged in user, and creates a cookie containing the temp file's name
    // and then redirects back to the page what called it.
    if (!empty($_REQUEST['base_path']) && !empty($_REQUEST['cookie_domain']) &&
        !empty($_REQUEST['destination'])) {
        $tmp_dir = substr($_SERVER['SCRIPT_FILENAME'],0,strpos($_SERVER['SCRIPT_FILENAME'],'user0_webauth.php')) . 'tmp';
        $tempnam = tempnam($tmp_dir,'');
        $ldap_vars = array();
        foreach ($_SERVER as $key => $value) {
            if (substr_compare($key, 'WEBAUTH_LDAP',0,12) === 0) {
                $subkey = substr($key,12);
		if (substr_compare($subkey, '_', 0, 1) !== 0) {
                    $subkey = '_' . $subkey;
                }
                $ldap_vars['R25_EXTERNAL'.$subkey] = $value;
            }
        }
        $ldap_vars['R25_EXTERNAL_DESTINATION'] = $_REQUEST['destination'];
        if (file_put_contents($tempnam,json_encode($ldap_vars)) !== false) {
		@chmod($tempnam,0660);
            $fname = substr($tempnam,strrpos($tempnam,'/')+1);
            setcookie('user0_webauth',$fname,0,$_REQUEST['base_path'],$_REQUEST['cookie_domain'],TRUE,TRUE);
        }
    	header('Location: https://'.$_SERVER['HTTP_HOST'].$_REQUEST['base_path'].$_REQUEST['destination']);
    } else {
	header('HTTP/1.0 404 Not Found');
        header('Location: https://'.$_SERVER['HTTP_HOST']);
    }
?>
