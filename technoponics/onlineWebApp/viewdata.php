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

	$sql = "SELECT * FROM main LIMIT 1000"; 
	$result = $conn->query($sql);

	if ($result->num_rows > 0) {
		echo "<table>";
		echo "<caption>Data Table</caption>";
		echo "<thead><tr><th>ID</th><th>Date</th><th>Time</th><th>Water Temperature</th><th>Air Temperature</th><th>pH Level</th></tr></thead>";
		echo "<tbody>";
		while ($row = $result->fetch_assoc()) {
			echo "<tr>";
			echo "<td>" . $row["ID"] . "</td>";
			echo "<td>" . $row["theDate"] . "</td>";
			echo "<td>" . $row["theTime"] . "</td>";
			echo "<td>" . $row["wTemp"] . "</td>";
			echo "<td>" . $row["aTemp"] . "</td>";
			echo "<td>" . $row["phLvl"] . "</td>";
			echo "</tr>";
		}
		echo "</tbody></table>";
	} else {
		echo "<div class='error'>0 results</div>";
	}

	$conn->close();
	?>

</div>

</body>
</html>
