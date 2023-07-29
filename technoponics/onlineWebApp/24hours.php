<!DOCTYPE html>
<html>
<head>
	<title>Database Table</title>
	<style>
		body {
			font-family: Arial, sans-serif;
			background-color: #f7f7f7;
		}
		table {
			border-collapse: collapse;
			width: 100%;
			background-color: #fff;
			box-shadow: 0 1px 3px rgba(0,0,0,0.2);
			margin-bottom: 20px;
		}
		th, td {
			border: 1px solid #dddddd;
			text-align: left;
			padding: 12px;
		}
		th {
			background-color: #f2f2f2;
			font-weight: bold;
			color: #666666;
		}
		tr:nth-child(even) td {
			background-color: #f2f2f2;
		}
		tr:hover td {
			background-color: #e6f7ff;
		}
		caption {
			font-size: 24px;
			font-weight: bold;
			margin-bottom: 10px;
		}
		.container {
			max-width: 1200px;
			margin: 0 auto;
			padding: 40px 20px;
		}
	</style>
</head>
<body>
      <a href='http://lormatechnoponics.tech/download.php'>DOWNLOAD ALL DATA FROM LORMATECHNOPONICS.TECH</a>
<div class="container">
	<h1>Database Table</h1>
	<?php
	include "conn.php";
	if ($conn->connect_error) {
		die("<div class='error'>Connection failed: " . $conn->connect_error . "</div>");
	}
	date_default_timezone_set("Asia/Hong_Kong");
	$last_24_hours = time() - 86400;
	$start_time = date("Y-m-d H:i:s", $last_24_hours);
	$end_time = date("Y-m-d H:i:s");
	$sql = "SELECT CONCAT(DATE_FORMAT(CONCAT(theDate, ' ', theTime), '%b %e %h'), '-', 
	DATE_FORMAT(DATE_ADD(CONCAT(theDate, ' ', theTime), INTERVAL 1 HOUR), '%h%p')) AS datetime, 
	ROUND(AVG(wTemp), 2) AS avg_wTemp, ROUND(AVG(aTemp), 2) AS avg_aTemp, ROUND(AVG(phLvl), 2) AS avg_phLvl 
	FROM main WHERE CONCAT(theDate, ' ', theTime) BETWEEN '$start_time' AND '$end_time' 
	GROUP BY DATE_FORMAT(CONCAT(theDate, ' ', theTime), '%Y-%m-%d %H'), HOUR(theTime)"; 
	$result = $conn->query($sql);
	if ($result) {
		echo "<table>
				<tr>
					<th>Datetime</th>
					<th>Avg. Water Temp</th>
					<th>Avg. Air Temp</th>
					<th>Avg. pH Level</th>
				</tr>";
		while ($row = mysqli_fetch_assoc($result)) {
			echo "<tr>
					<td>".$row['datetime']."</td>
					<td>".$row['avg_wTemp']."</td>
					<td>".$row['avg_aTemp']."</td>
					<td>".$row['avg_phLvl']."</td>
				</tr>";
		}
		echo "</table>";
	} else {
		echo "Error: " . mysqli_error($connection);
	}

	$conn->close();
	?>

</div>

</body>
</html>
