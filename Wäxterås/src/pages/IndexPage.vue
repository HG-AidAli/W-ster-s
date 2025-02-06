<template>
  <q-page>
    <div v-if="!isControlMode" class="the_whole_thing">
      <div class="row">
        <div class="servo-control">
          <h5>Servo Angle</h5>
          <q-slider v-model="servoAngle" :min="0" :max="90" label="Servo Angle" @change="updateServoAngle" />
        </div>
        <div class="pump-control">
          <h5>Pump Speed</h5>
          <q-slider v-model="pumpSpeed" :min="0" :max="3000" label="Pump Speed" @change="updatePumpSettings"/>
        </div>
        <div class="motor-control">
          <h5>Motor Speed</h5>
          <q-slider v-model="motorSpeed" :min="0" :max="3000" label="Motor Speed" @change="updateMotorSettings" />
        </div>

      </div>
      <div class="sensor-data">
        <p><strong>Temperature:</strong> {{ sensorData.temperature }}°C</p>
        <p><strong>Humidity:</strong> {{ sensorData.humidity }}%</p>
        <q-btn @click="toggleChart" size="sm" color="primary" >
          Show {{ isTemperatureChart ? "Humidity" : "Temperature" }} Data
        </q-btn>

        <apexchart  style="background-color:rgba(40,40,40,1);" type="line" :options="chartOptions" :series="chartSeries" />

        <div class="historical-data">
          <q-select
            dark
            rounded
            standout
            class="select-date"
            v-model="selectedDate"
            :options="availableDates"
            label="Select Date"
          />
          <q-btn @click="showHistoricalGraphs" color="secondary" class="q-mt-md">
            {{ isHistoricalGraphMode ? "Exit" : "Show Historical Graphs" }}
          </q-btn>
        </div>

        <div v-if="historicalGraphsVisible" class="historical-graphs">
          <h5>Historical Temperature Data</h5>
          <apexchart
            type="line"
            :options="chartOptions"
            :series="temperatureChartSeries"
          />
          <h5>Historical Humidity Data</h5>
          <apexchart
            type="line"
            :options="chartOptions"
            :series="humidityChartSeries"
          />
        </div>
      </div>
    </div>

    <div v-if="isControlMode">
      <div class="chart-container">
        <q-btn @click="toggleChart" size="sm" color="primary">
          Show {{ isTemperatureChart ? "Humidity" : "Temperature" }} Data
        </q-btn>

        <apexchart
          type="line"
          :options="chartOptions"
          :series="chartSeries"
          class="sensor-chart"
        />

        <div class="status-boxes">
          <div class="status-box motor-speed">
            <p>Motor Speed: {{ convertMotorUsageToProcent(motorSpeed)}}</p>
          </div>
          <div class="status-box servo-angle">
            <p>Servo Angle: {{ convertServoAngleToProcent(servoAngle)}}</p>
          </div>
          <div class="status-box pump-speed">
            <p>Pump Speed: {{ convertPumpUsageToProcent(pumpSpeed)}}</p>
          </div>
        </div>
      </div>
    </div>


    <q-btn @click="fixControlToggleProblems" size="sm" color="accent">
      {{ isControlMode ? "Exit Control Mode" : "Control" }}
    </q-btn>
  </q-page>
</template>











<script>
//Importerar viktiga grejer både för firebase och för att kunna rita grafer (apexchart är använd i detta fall)
import { initializeApp, getApps } from 'firebase/app';
import { getDatabase, ref, onValue, set } from 'firebase/database';
import VueApexCharts from "vue3-apexcharts";

export default {
  components: {
    apexchart: VueApexCharts,
  },
  data() {
    return {
      targetTemperature: 22,
      targetHumidity: 78,
      servoAngle: 0,
      motorSpeed: 0,
      pumpSpeed: 0,
      motorDirection: false,
      sensorData: {
        temperature: null,
        humidity: null,
      },

      temperatureData: Array(50).fill(26),
      humidityData: Array(50).fill(30),
      isTemperatureChart: true,
      availableDates: [],
      historicalData: [],
      selectedDate: null,
      historicalGraphsVisible: false,
      temperatureChartSeries: [],
      humidityChartSeries: [],
      isHistoricalGraphMode: false,
      isControlMode: false,
      temperatureIsControlled: true,
      humidityIsControlled: true,
      pidInterval: null,
      tempErrorSum: 0,
      tempLastError: 0,
      humidityErrorSum: 0,
      humidityLastError: 0,
      humidityErrorSumForPump: 0,
      humidityLastErrorForPump: 0,
    };
  },
  computed: {
      chartOptions() {
    return {
      chart: {
        id: "sensor-data",
        animations: {
          enabled: true,
          easing: "easeinout",
          speed: 800,
        },
      },
      colors: ["#A4F378", "#20C997", "#FFC107", "#FF5733"],

      xaxis: {
        categories: Array.from({ length: 50}, (_, i) => i + 1),
      },
      yaxis: {
        title: { text: "Value" },
      },
      markers: {
        size: 5,
      },
      annotations: {
        yaxis: [
          {
            y: this.targetHumidity,
            borderColor: '#FF0000',
            label: {
              text: `Setpoint: ${this.targetHumidity}°C`,
              style: {
                color: '#FF0000',
                background: '#ffffff',
              },
            },
          },
          {
            y: this.targetTemperature,
            borderColor: '#00FF00',
            label: {
              text: `Setpoint: ${this.targetTemperature}°C`,
              style: {
                color: '#00FF00',
                background: '#ffffff',
              },
            },
          },
        ],
      },
    };
  },
    chartSeries() {
      return [
        {
          name: this.isTemperatureChart ? "Temperature" : "Humidity",
          data: this.isTemperatureChart ? this.temperatureData : this.humidityData,
        },
      ];
    },
  },
  methods: {

    convertMotorUsageToProcent(motorUsage) {
      if (motorUsage < 120) {
        return "0%";
      }

      const procent = Math.round(((motorUsage - 120) / (3000 - 120) ) * 100);
      return procent + "%";
    },
    convertPumpUsageToProcent(pumpUsage) {
      if (pumpUsage < 40) {
        return "0%";
      }

      const procent = Math.round(((pumpUsage - 40) / (300 - 40) ) * 100);
      return procent + "%";
    },

    convertServoAngleToProcent(ServoUsage){
      if (ServoUsage < 0) {
        return "0%";
      }

      const procent = Math.round(((ServoUsage) / (90) ) * 100);
      return procent + "%";
    },
    fetchHistoricalData() {
      const an_array = [];
      const longTermInfo = ref(this.database, "long-term-info");
      onValue(longTermInfo, (snapshot) => {
        const data = snapshot.val();
        for (const month in data) {
          onValue(ref(this.database, "long-term-info/" + month), (snapshot) => {
            const fetchedDay = snapshot.val();
            for (const day in fetchedDay) {
              an_array.push(month + "/" + day);
            }
          });
        }
      });
      this.availableDates = an_array;
    },
    fetchSensorData() {
      const sensorRef = ref(this.database, "environment");
      onValue(sensorRef, (snapshot) => {
        const data = snapshot.val();
        if (data) {
          this.sensorData = {
            temperature: data.temperature || "N/A",
            humidity: data.humidity || "N/A",
          };

          if (this.sensorData.temperature !== null) {
            this.temperatureData.push(this.sensorData.temperature);
            if (this.temperatureData.length > 50) this.temperatureData.shift();
          }

          if (this.sensorData.humidity !== null) {
            this.humidityData.push(this.sensorData.humidity);
            if (this.humidityData.length > 50) this.humidityData.shift();
          }

          this.updateChartSeries();
        }
      }, (error) => {
        console.error("Failed to fetch sensor data:", error);
      });
    },
    toggleChart() {
      this.isTemperatureChart = !this.isTemperatureChart;
      this.updateChartSeries();
    },

    updateChartSeries() {
      if (this.isTemperatureChart) {
        this.chartSeries = [
          {
            name: "Temperature",
            data: this.temperatureData,
          },
          {
            name: "Setpoint Temperature",
            data: Array(this.temperatureData.length).fill(this.targetTemperature),
          },
        ];
      } else {
        this.chartSeries = [
          {
            name: "Humidity",
            data: this.humidityData,
          },
          {
            name: "Setpoint Humidity",
            data: Array(this.humidityData.length).fill(this.targetHumidity),
          },
        ];
      }
    },
    toggleControlMode() {
      this.isControlMode = !this.isControlMode;
      if (this.isControlMode) {
        this.startPIDControl();
      } else {
        this.stopPIDControl();
      }
    },
    startPIDControl() {
      this.pidInterval = setInterval(() => {
        this.performPIDControl();
      }, 4000);
    },
    stopPIDControl() {
      clearInterval(this.pidInterval);
      this.pidInterval = null;
    },
    performPIDControl() {
      //Hita anledning till: Varför är det så jäckla trög?
      const tempKp = 100;
      const tempKi = 10;
      const tempKd = 1;
      const temperatureError = this.sensorData.temperature - this.targetTemperature;
      this.tempErrorSum += temperatureError;
      const tempDeltaError = temperatureError - this.tempLastError;
      const motorControlSignal = tempKp * temperatureError + tempKi * this.tempErrorSum + tempKd * tempDeltaError;
      this.tempLastError = temperatureError;

      if (this.sensorData.temperature > this.targetTemperature) {
        this.temperatureIsControlled = false;
        this.servoAngle = 90;
        this.motorSpeed = Math.round(Math.min(Math.max(120, motorControlSignal), 3000));
      } else {
        this.temperatureIsControlled = true;
        this.motorSpeed = 0;
        this.tempErrorSum = 0;
      }
      set(ref(this.database, "servo/angle"), this.servoAngle);
      set(ref(this.database, "motor/speed"), this.motorSpeed);


      const humSetPoint = 78;
      const humKp = 7;
      const humKi = 0.7;
      const humKd = 0.07;
      const humidityError = this.sensorData.humidity - humSetPoint;
      this.humidityErrorSum += humidityError;
      const humidityDeltaError = humidityError - this.humidityLastError;
      const servoControlSignal = humKp * humidityError + humKi * this.humidityErrorSum + humKd * humidityDeltaError;
      this.humidityLastError = humidityError;

      //PLEASE TUNE THE FREAKING PID CONSTS...
      const pumpKp = 1;
      const pumpKi = 0.1;
      const pumpKd = 0.01;
      const humidityErrorForPump = -(this.sensorData.humidity - humSetPoint);
      this.humidityErrorSumForPump += humidityErrorForPump;
      const humidityDeltaErrorForPump = humidityErrorForPump - this.humidityLastErrorForPump;
      const pumpControlSignal = pumpKp * humidityErrorForPump + pumpKi * this.humidityErrorSumForPump + pumpKd * humidityDeltaErrorForPump;

      if (this.sensorData.humidity > this.targetHumidity) {
        if (this.temperatureIsControlled == true) {
          this.humidityIsControlled = false;
          this.servoAngle = Math.round(Math.min(Math.max(0, servoControlSignal), 90));
        }
        else {
          this.servoAngle = 90;
        }
      }

      else {

        this.humidityIsControlled = true;
        this.humidityErrorSum = 0;
      }

      set(ref(this.database, "servo/angle"), this.servoAngle);

      if (this.sensorData.humidity < this.targetHumidity) {
        if (this.temperatureIsControlled == true) {

          this.pumpSpeed= Math.round(Math.min(Math.max(40, pumpControlSignal), 300));
          this.humidityIsControlled = false;

        }

        else {

          this.humidityIsControlled = true;
          this.pumpSpeed = 0;

        }

      }
      set(ref(this.database, "pump/speed"), this.pumpSpeed);


      if (this.sensorData.temperature > this.targetTemperature){
        this.temperatureIsControlled = false;
      }
      else {
        this.temperatureIsControlled = true;
        if (this.sensorData.humidity > this.targetHumidity) {
          set(ref(this.database, "pump/speed"), 0);
        }
        if (this.sensorData.humidity < this.targetHumidity) {
          set(ref(this.database, "servo/angle"), 0);
        }
      }
    },
    fixControlToggleProblems() {

      this.nollifyTheMotorValues();
      this.toggleControlMode();

    },
    updateServoAngle() {
      set(ref(this.database, "servo/angle"), this.servoAngle);
    },
    updateMotorSettings() {
      set(ref(this.database, "motor"), {
        speed: this.motorSpeed,
        direction: this.motorDirection,
      });
    },
    updatePumpSettings() {
      set(ref(this.database, "pump/speed"), this.pumpSpeed);
    },
    nollifyTheMotorValues(){

        this.motorSpeed = 0;
        this.pumpSpeed = 0;
        this.servoAngle = 0;
        set(ref(this.database, "pump/speed"), this.pumpSpeed);
        set(ref(this.database, "motor/speed"), this.motorSpeed);
        set(ref(this.database, "servo/angle"), this.servoAngle);
    },
    showHistoricalGraphs() {
      this.isHistoricalGraphMode = !this.isHistoricalGraphMode;

      if (this.isHistoricalGraphMode && this.selectedDate) {
        const [month, day] = this.selectedDate.split("/");
        const dataPath = `long-term-info/${month}/${day}`;
        const temperatureArray = [];
        const humidityArray = [];
        const dayRef = ref(this.database, dataPath);

        onValue(dayRef, (snapshot) => {
          const hourlyData = snapshot.val();
          if (hourlyData) {
            Object.keys(hourlyData).forEach((hour) => {
              const temp = hourlyData[hour]?.temp || null;
              const hum = hourlyData[hour]?.hum || null;

              if (temp !== null) temperatureArray.push(temp);
              if (hum !== null) humidityArray.push(hum);
            });

            this.temperatureChartSeries = [
              {
                name: "Temperature",
                data: temperatureArray,
              },
            ];
            this.humidityChartSeries = [
              {
                name: "Humidity",
                data: humidityArray,
              },
            ];

            this.historicalGraphsVisible = true;
          } else {
            console.log("No data available for the selected date.");
          }
        });
      } else {
        this.historicalGraphsVisible = false;
      }
    },
  },
  mounted() {
    const firebaseConfig = {
      apiKey: "AIzaSyD6O7hQ2SUBCajbVClFEMH5fzXKfrRH0Z0",
      authDomain: "waxteras-8636d.firebaseapp.com",
      databaseURL:
        "https://waxteras-8636d-default-rtdb.europe-west1.firebasedatabase.app",
      projectId: "waxteras-8636d",
      storageBucket: "waxteras-8636d.firebasestorage.app",
      messagingSenderId: "699156359214",
      appId: "1:699156359214:web:6d98ab6b52b65313b957bf",
    };

    const app = getApps().length === 0 ? initializeApp(firebaseConfig) : getApps()[0];
    this.database = getDatabase(app);
    this.fetchHistoricalData();
    this.fetchSensorData();
    setInterval(this.fetchHistoricalData, 5000);
    setInterval(this.fetchSensorData, 5000);
  },
};
</script>


























<style>
html, body {
  background-color: rgba(50,50,50,1) !important;
  color: white;
  margin: 0;
  padding: 0;
  min-height: 100vh;
  width: 100%;
}


.row {
  color :white;
  display: flex;
  justify-content:space-around;
  margin-bottom: 1rem;
}
.servo-control {
  width: 45%;
  padding: 1rem;
}

.select-date {
  background-color: rgba(50,50,50,1) !important;
  color: rgba(255,180,80,1) !important;

}
.pump-control {
  width: 45%;
  padding: 1rem;
}
.motor-control {
  width: 45%;
  padding: 1rem;
}
.sensor-data {
  background: rgba(40,40,40,1);
  padding: 2rem;
  border-radius: 8px;
  text-align: center;
  margin-top: 2rem;
  height: 400px;
}

.pump-control h5,
.servo-control h5,
.motor-control h5 {
  font-size: 0.9rem;
  margin-bottom: 0.5rem;
  text-align: center;
}
.q-btn {
  font-size: 0.8rem;
}
.historical-data {
  background-color: rgba(50,50,50,1);
  margin-top: 2rem;
}

.historical-graphs{

}
.status-boxes {
  display: flex;
  justify-content: space-between;
  margin-top: 20px;
}

.status-box {
  width: 80px;
  height: 80px;
  display: flex;
  justify-content: center;
  align-items: center;
  color: rgb(41, 21, 21);
  font-weight: bold;
}
.select-date {
  color:black;
}
.motor-speed {
  background-color:rgba(180, 255, 80, 1);
}

.servo-angle {
  background-color: rgba(255, 180, 80, 1);
}

.pump-speed {
  background-color: rgba(80, 180, 255);
}
</style>
