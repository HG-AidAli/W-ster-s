<template>
  <q-page>
    <div v-if="!isControlMode">
      <div class="row">
        <div class="col-6 servo-control">
          <h5>Servo Angle</h5>
          <q-slider v-model="servoAngle" :min="0" :max="90" label @change="updateServoAngle" />
        </div>
        <div class="col-6 motor-control">
          <h5>Motor Control</h5>
          <q-slider v-model="motorSpeed" :min="0" :max="3000" label="Motor Speed" @change="updateMotorSettings" />
        </div>
      </div>
      <div class="sensor-data">
        <p><strong>Temperature:</strong> {{ sensorData.temperature }}°C</p>
        <p><strong>Humidity:</strong> {{ sensorData.humidity }}%</p>
        <q-btn @click="showHistoricalGraphs">Testing functions with this button</q-btn>
        <q-btn @click="toggleChart" size="sm" color="primary">
          Show {{ isTemperatureChart ? "Humidity" : "Temperature" }} Data
        </q-btn>

        <apexchart type="line" :options="chartOptions" :series="chartSeries" />

        <div class="historical-data">
          <q-select

            v-model="selectedDate"
            :options="availableDates"
            label="Select Date"
            onselect=""

          />
          <q-btn @click="showHistoricalGraphs" color="secondary" class="q-mt-md">
            Show Historical Events
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

    <q-btn @click="toggleControlMode" size="sm" color="accent">
      {{ isControlMode ? "Exit Control Mode" : "Control" }}
    </q-btn>
  </q-page>
</template>
























<script>
import { initializeApp, getApps } from 'firebase/app';
import { getDatabase, ref, onValue, set } from 'firebase/database';
import VueApexCharts from "vue3-apexcharts";

export default {
  components: {
    apexchart: VueApexCharts,
  },
  data() {
    return {
      servoAngle: 0,
      motorSpeed: 0,
      motorDirection: false,
      sensorData: {
        temperature: null,
        humidity: null,
      },
      temperatureData: Array(10).fill(26),
      humidityData: Array(10).fill(30),
      isTemperatureChart: true,
      availableDates:[] ,
      historicalData: [],
      selectedDate: null,
      historicalGraphsVisible: false,
      temperatureChartSeries: [],
      humidityChartSeries: [],
      chartOptions: {
        chart: {

          id: "sensor-data",
          animations: {
            enabled: true,
            easing: "easeinout",
            speed: 800,
          },
        },
        xaxis: {
          categories: Array.from({ length: 10 }, (_, i) => i + 1),
        },
          id: "historical-data",
          animations: {
            enabled: true,
            easing: "easeinout",
            speed: 800,
          },
          toolbar: {
            show: false,
          },
      },
        xaxis: {
          type: "datetime",
          title: { text: "Date" },
        },
        yaxis: {
          title: { text: "Value" },
        },
        markers: {
          size: 5,
        },

      isControlMode: false,
      temperatureIsControlled : true,
      humidityIsControlled: true,
      targetTemperature: 26,
      targetHumidity: 30,
      pidInterval: null,
      tempErrorSum: 0,
      tempLastError: 0,
      humidityErrorSum: 0,
      humidityLastError: 0,
    };
  },
  computed: {
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
    showHistoricalGraphs() {
  if (!this.selectedDate) {
    console.log("No date selected.");
    return;
  }

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

      console.log("Temperature Array: " + temperatureArray);
      console.log("Humidity Array: " + humidityArray);
    } else {
      console.log("No data available for the selected date.");
    }
  });
},

    toggleChart() {
      this.isTemperatureChart = !this.isTemperatureChart;
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
        this.motorSpeed = Math.round(Math.min(Math.max(500, motorControlSignal), 3000));
      } else {
        this.temperatureIsControlled = true;
        this.motorSpeed = 0;
        this.tempErrorSum = 0;
      }
      set(ref(this.database, "servo/angle"), this.servoAngle);
      set(ref(this.database, "motor/speed"), this.motorSpeed);


      const humSetPoint = 30;
      const humKp = 1;
      const humKi = 0.1;
      const humKd = 0.01;
      const humidityError = this.sensorData.humidity - humSetPoint;
      this.humidityErrorSum += humidityError;
      const humidityDeltaError = humidityError - this.humidityLastError;
      const servoControlSignal = humKp * humidityError + humKi * this.humidityErrorSum + humKd * humidityDeltaError;
      this.humidityLastError = humidityError;

      if (this.sensorData.humidity > this.targetHumidity) {
        if (this.temperatureIsControlled == true) {
          this.humidityIsControlled = false;
          this.servoAngle = Math.round(Math.min(Math.max(0, servoControlSignal), 90));
        }
      } else {
        this.humidityIsControlled = true;
        this.servoAngle = 0;
      }
      set(ref(this.database, "servo/angle"), this.servoAngle);
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

    fetchHistoricalData() {
      const an_array = [];
      const longTermInfo = ref(this.database, "long-term-info");
      onValue(
        longTermInfo,
        (snapshot) => {
          const data = snapshot.val();
          for(const month in data) {
            onValue(
              ref(this.database, "long-term-info/" + month),
              (snapshot) => {
                const fetchedDay = snapshot.val();
                for (const day in fetchedDay) {
                  an_array.push(month + "/" + day)
                }
              }
            )
          }

        }
      )
      this.availableDates = an_array;
    },


    fetchSensorData() {
      const sensorRef = ref(this.database, "environment");
      onValue(
        sensorRef,
        (snapshot) => {
          const data = snapshot.val();
          if (data) {
            this.sensorData = {
              temperature: data.temperature || "N/A",
              humidity: data.humidity || "N/A",
            };

            if (data.temperature !== undefined) {
              this.temperatureData.push(data.temperature);
              if (this.temperatureData.length > 10) this.temperatureData.shift();
            }

            if (data.humidity !== undefined) {
              this.humidityData.push(data.humidity);
              if (this.humidityData.length > 10) this.humidityData.shift();
            }
          }
        },
        (error) => {
          console.error("Failed to fetch sensor data:", error);
        }
      );
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
    setInterval(this.fetchHistoricalData, 5000);
    setInterval(this.fetchSensorData, 5000);
  },
};
</script>











<style>
.row {
  display: flex;
  justify-content: space-between;
  margin-bottom: 1rem;
}
.servo-control {
  width: 45%;
  padding: 1rem;
}
.motor-control {
  width: 45%;
  padding: 1rem;
}
.sensor-data {
  background: #f5f5f5;
  padding: 2rem;
  border-radius: 8px;
  text-align: center;
  margin-top: 2rem;
  height: 400px;
}
.servo-control h5,
.motor-control h5 {
  font-size: 0.9rem;
  margin-bottom: 0.5rem;
}
.q-btn {
  font-size: 0.8rem;
}
.historical-data {
  margin-top: 2rem;
}
</style>
