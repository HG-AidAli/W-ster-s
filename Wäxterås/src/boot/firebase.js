import { boot } from "quasar/wrappers";
import { initializeApp, getApps } from "firebase/app";
import { getDatabase } from "firebase/database";
import { VueFire, VueFireAuth } from "vuefire";

const firebaseConfig = {

  apiKey: "AIzaSyD6O7hQ2SUBCajbVClFEMH5fzXKfrRH0Z0",

  authDomain: "waxteras-8636d.firebaseapp.com",

  databaseURL: "https://waxteras-8636d-default-rtdb.europe-west1.firebasedatabase.app",

  projectId: "waxteras-8636d",

  storageBucket: "waxteras-8636d.firebasestorage.app",

  messagingSenderId: "699156359214",

  appId: "1:699156359214:web:6d98ab6b52b65313b957bf"

};


const apps = getApps();
let firebaseApp;
if (!apps.length) {
  firebaseApp = initializeApp(firebaseConfig);
} else {
  firebaseApp = apps[0];
}

const db = getDatabase(firebaseApp);

export default boot(async ({ app }) => {
  app.use(VueFire, {
    firebaseApp,
    modules: [
      VueFireAuth(),
    ],
  });
});

export { db };
