<?php
    ini_set('display_errors', 1);
    ini_set('display_startup_errors', 1);
    error_reporting(E_ALL);

    $date = $_GET["date"];
    $time = $_GET["time"];
    $wTemp = $_GET["wTemp"];
    $aTemp = $_GET["aTemp"];
    $phLvl = $_GET["phLvl"];

    if(empty($_GET["date"]) || empty($_GET["time"]) || empty($_GET["wTemp"]) || empty($_GET["aTemp"]) || empty($_GET["phLvl"])) {
        die("Error: Invalid request parameters");
    }

    include "conn.php";

    if ($conn->connect_error) {
        die("Connection failed: " . $conn->connect_error);
    }

    $sql = "INSERT INTO main (theDate, theTime, wTemp, aTemp, phLvl) VALUES ('$date', '$time', $wTemp, $aTemp, $phLvl)";

    if ($conn->query($sql) === TRUE) {
        echo "New record created successfully";
    } else {
        echo "Error: " . $sql . " => " . $conn->error;
    }

    $conn->close();
?>