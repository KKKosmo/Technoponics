<?php
    require("PHPMailer/src/PHPMailer.php");
    require("PHPMailer/src/SMTP.php");


    $mail = new PHPMailer\PHPMailer\PHPMailer();

    $date = $_GET["date"];
    $time = $_GET["time"];
    $wTemp = $_GET["wTemp"];
    $aTemp = $_GET["aTemp"];
    $phLvl = $_GET["phLvl"];
    $mailTo = $_GET["receiver"];
    $danger = $_GET["danger"];
    if($danger == "wHigh"){
        $danger = "WARNING: Water Temperature too high, <br>";
    }
    else if($danger == "wLow"){
        $danger = "WARNING: Water Temperature too low, <br>";
    } 
    else if($danger == "aHigh"){
        $danger = "WARNING: Air Temperature too high, <br>";
    }
    else if($danger == "aLow"){
        $danger = "WARNING: Air Temperature too low, <br>";
    }
    else if($danger == "pHigh"){
        $danger = "WARNING: pH Level too high, <br>";
    }
    else if($danger == "pLow"){
        $danger = "WARNING: pH Level too low, <br>";
    }
    else if($danger == "sdCardToday"){
        $danger = "WARNING: SD CARD FAILED TO OPEN TODAY's FILE, <br>";
    }
    else if($danger == "sdCardATD"){
        $danger = "WARNING: SD CARD FAILED TO OPEN ALL TIME DATA FILE, <br>";
    }
    $body = $danger . " " . $date . " " . $time . " <br>Water Temperature: " . $wTemp . " <br>Air Temperature: " . $aTemp . " <br>pH Level: " . $phLvl;


    $mail->isSMTP();

    $mail->Host="mail.smtp2go.com";
    $mail->SMTPAuth = true;

    $mail->Username = "";
    $mail->Password = "";

    $mail->SMTPSecure = "tls";

    $mail->Port = "2525";

    $mail->From = "@gmail.com";
    $mail->FromName = "Technoponics";

    $mail->addAddress($mailTo, "Farmer");

    $mail->Subject = "Test Email Notification";

    $mail->Body = $body;
    $mail->AltBody = "AltBody";

    if(!$mail->send()){
        echo "Mailer Error:". $mail->ErrorInfo;
    }
    else{
        echo "Message has been sent";
    }

?>
