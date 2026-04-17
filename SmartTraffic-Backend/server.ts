import express from 'express';
import mongoose from 'mongoose';
import cors from 'cors';

const app = express();
app.use(express.json());
app.use(cors());

// MongoDB connection string
const MONGO_URI = 'mongodb+srv://CG2271:197772Czz1664@cluster.vwct9sy.mongodb.net/SmartTraffic?appName=Cluster';

// 1. Connect to Database
mongoose.connect(MONGO_URI)
  .then(() => console.log('MongoDB Connected!'))
  .catch(err => console.error('MongoDB Connection Error:', err));

// 2. Define Data Schema
const TelemetrySchema = new mongoose.Schema({
  sessionId: { type: String, required: true },
  speedrange: { type: Number, required: true },
  timestamp: { type: Date, default: Date.now } 
});
const Telemetry = mongoose.model('Telemetry', TelemetrySchema);

// 3. POST API: Receive telemetry data from ESP32
app.post('/api/telemetry', async (req, res) => {
  try {
    const { sessionId, speedrange } = req.body;
    const newData = new Telemetry({ sessionId, speedrange });
    await newData.save();
    
    console.log(`Received ESP32 Data: Session=${sessionId}, speedrange=${speedrange}`);
    res.status(200).send('Data saved successfully');
  } catch (error) {
    console.error('Data Save Error:', error);
    res.status(500).send('Internal Server Error');
  }
});

// 4. GET API: Provide data to Frontend Dashboard
app.get('/api/telemetry', async (req, res) => {
  try {
    // Fetch the latest 30 records sorted by timestamp descending
    const data = await Telemetry.find().sort({ timestamp: -1 }).limit(30);
    res.status(200).json(data);
  } catch (error) {
    console.error('Fetch Data Error:', error);
    res.status(500).json({ error: 'Failed to fetch data' });
  }
});

// 5. Start Server
app.listen(3000, '0.0.0.0', () => {
  console.log('Server started! Listening on port 3000...');
});