function download(){
    window.location.href = 'download.php';
}

var waterTempChart, airTempChart, phLevelChart;

function fetchData() {
  fetch('fetch_data.php')
    .then(function (response) {
      return response.json();
    })
    .then(function (data) {
      renderTable(data);
      renderGraphs(data);
    })
    .catch(function (error) {
      console.log('Error fetching data:', error);
    });
}

function renderGraphs(data) {
  var graphData = data.graphData;
  var dates = graphData.map(function (entry) {
    return entry.date;
  });
  var waterTemps = graphData.map(function (entry) {
    return entry.water_temperature;
  });
  var airTemps = graphData.map(function (entry) {
    return entry.air_temperature;
  });
  var phLevels = graphData.map(function (entry) {
    return entry.ph_level;
  });

  var waterTempCtx = document.getElementById('waterTempChart').getContext('2d');
  waterTempChart = new Chart(waterTempCtx, {
    type: 'line',
    data: {
      labels: dates,
      datasets: [{
        label: 'Water Temperature',
        data: waterTemps,
        borderColor: 'blue',
        fill: false
      }]
    },
    options: {
      responsive: true,
      maintainAspectRatio: false
    }
  });

  var airTempCtx = document.getElementById('airTempChart').getContext('2d');
  airTempChart = new Chart(airTempCtx, {
    type: 'line',
    data: {
      labels: dates,
      datasets: [{
        label: 'Air Temperature',
        data: airTemps,
        borderColor: 'red',
        fill: false
      }]
    },
    options: {
      responsive: true,
      maintainAspectRatio: false
    }
  });

  var phLevelCtx = document.getElementById('phLevelChart').getContext('2d');
  phLevelChart = new Chart(phLevelCtx, {
    type: 'line',
    data: {
      labels: dates,
      datasets: [{
        label: 'pH Level',
        data: phLevels,
        borderColor: 'green',
        fill: false
      }]
    },
    options: {
      responsive: true,
      maintainAspectRatio: false
    }
  });
}

function renderTable(data) {
  var tableData = data.tableData;
  
var tableBody = document.querySelector('#tableBody');


  tableBody.innerHTML = '';

  tableData.forEach(function (entry) {
    var row = document.createElement('tr');

    var dateCell = document.createElement('td');
    dateCell.textContent = entry.date;
    row.appendChild(dateCell);

    var timeCell = document.createElement('td');
    timeCell.textContent = entry.time;
    row.appendChild(timeCell);
    
    var waterTempCell = document.createElement('td');
    waterTempCell.textContent = entry.water_temperature;
    row.appendChild(waterTempCell);

    var airTempCell = document.createElement('td');
    airTempCell.textContent = entry.air_temperature;
    row.appendChild(airTempCell);

    var phLevelCell = document.createElement('td');
    phLevelCell.textContent = entry.ph_level;
    row.appendChild(phLevelCell);
    
    tableBody.appendChild(row);

    });
    }
    
    fetchData();