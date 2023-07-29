<?php
require_once 'conn.php'; 

$start = $_GET['start'];
$end = $_GET['end'];

try {
    if (!empty($start) && !empty($end)) {
        $stmt = $conn->prepare("SELECT theDate AS date, theTime AS time, aTemp AS air_temperature, wTemp AS water_temperature, phLvl AS ph_level FROM main WHERE theDate >= ? AND theDate <= ? LIMIT 1000");
        $stmt->bind_param("ss", $start, $end);
    } else {
        $stmt = $conn->prepare("SELECT theDate AS date, theTime AS time, aTemp AS air_temperature, wTemp AS water_temperature, phLvl AS ph_level FROM main LIMIT 1000");
    }
    $stmt->execute();
    $result = $stmt->get_result();
    $data = array();
    while ($row = $result->fetch_assoc()) {
        $data[] = $row;
    }
} catch (PDOException $e) {
    die("Error retrieving data: " . $e->getMessage());
}

$graphData = array_map(function($entry) {
    return [
        'date' => $entry['date'],
        'time' => $entry['time'],
        'air_temperature' => $entry['air_temperature'],
        'water_temperature' => $entry['water_temperature'],
        'ph_level' => $entry['ph_level']
    ];
}, $data);

$tableData = array_map(function($entry) {
    return [
        'date' => $entry['date'],
        'time' => $entry['time'],
        'air_temperature' => $entry['air_temperature'],
        'water_temperature' => $entry['water_temperature'],
        'ph_level' => $entry['ph_level']
    ];
}, $data);

header('Content-Type: application/json');
echo json_encode(['graphData' => $graphData, 'tableData' => $tableData]);
?>
