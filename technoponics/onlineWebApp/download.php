<?php
    include "conn.php";
if (!$conn) {
  die("Connection failed: " . mysqli_connect_error());
}
$sql = "SELECT * FROM main";
if(!empty($_GET["sTime"]) && !empty($_GET["eTime"]) && !empty($_GET["sDate"]) && !empty($_GET["eDate"]) ){
  $start_time = $_GET['sDate'] . ' ' . $_GET['sTime'] . ':00';
  $end_time = $_GET['eDate'] . ' ' . $_GET['eTime'] . ':00';
  $sql = "SELECT * FROM main WHERE STR_TO_DATE(CONCAT(theDate, ' ', theTime), '%Y-%m-%d %H:%i:%s') BETWEEN '$start_time' AND '$end_time'";
}


$result = mysqli_query($conn, $sql);

header('Content-Type: text/csv');
header('Content-Disposition: attachment; filename="main.csv"');
$header = array('Row ID', 'Date', 'Time', 'Water Temperature', 'Air Temperature', 'pH Level'); 
echo implode(',', $header) . "\n";

$output = fopen('php://output', 'w');
while ($row = $result->fetch_assoc()) {
    fputcsv($output, $row);
}
fclose($output);

$conn->close();
?>