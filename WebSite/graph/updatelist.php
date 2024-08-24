<?php
    $ToAddr = "graphupdate-subscribe@yahoogroups.com";
    $Body = "";
    $FromAddr = filter_var($_POST['email'], FILTER_SANITIZE_EMAIL);
    $Subject = "";
    $RedirectSuccess = "/subscribed";
    $RedirectFail = "/invalid-email";
    $Header = "From: " . $FromAddr . "\r\n";

    if(!filter_var($FromAddr, FILTER_VALIDATE_EMAIL))
    {
      header("Location: " . $RedirectFail);
    }
    else if(mail($ToAddr, $Subject, $Body, $Header))
    {
   	header("Location: " . $RedirectSuccess);
    }
    else
    {
	   echo "Error sending mail";
    }
?>
