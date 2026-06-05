const express = require("express");
const { exec } = require("child_process");
const path = require("path");

const fs = require("fs");
const axios = require("axios");

const app = express();
const PORT = 8000;

app.use(express.static(__dirname));

function makeSpeech(text) {
  return new Promise((resolve, reject) => {
    const finalPath = path.join(__dirname, "speech.wav");

    const safeText = text.replace(/"/g, "'");

    const command =
      `edge-tts --voice en-GB-RyanNeural ` +
      `--text "${safeText}" --write-media temp.mp3`;

    exec(command, (err, stdout, stderr) => {
      if (err) {
        console.log("Edge TTS error:", stderr);
        return reject(err);
      }

      exec(
        `ffmpeg -y -i temp.mp3 -af "loudnorm,volume=7.0" -ac 1 -ar 16000 -sample_fmt s16 "${finalPath}"`,
        (err2, stdout2, stderr2) => {
          if (err2) {
            console.log("FFmpeg error:", stderr2);
            return reject(err2);
          }

          resolve();
        }
      );
    });
  });
}

async function askOllama(question) {
  const response = await axios.post("http://localhost:11434/api/generate", {
    model: "phi4-mini",
    prompt: `you are Kiwi, a table top voice assistant, reply short in under 20 words.
if the user commands you something, do as comanded
your favoruite color is green,
User: ${question}
Kiwi:`,
    stream: false,
    options: {
      temperature: 0.6,
      num_predict: 50
    }
  });

  return response.data.response?.trim() || "I got no reply.";
}

function transcribeAudio() {
  return new Promise((resolve, reject) => {
    exec(`whisper input.wav --model base --language English --output_format txt`, (err, stdout, stderr) => {
      if (err) {
        console.log("Whisper error:", stderr);
        return reject(err);
      }

      const textPath = path.join(__dirname, "input.txt");
      const text = fs.readFileSync(textPath, "utf8").trim();

      console.log("Whisper text:", text);
      resolve(text || "user didn't say anything");
    });
  });
}


app.get("/test", async (req, res) => {
  const question = req.query.q || "Say hello in one short sentence.";

  try {
    const reply = await askOllama(question);
    await makeSpeech(reply);
    res.send(reply);
  } catch (err) {
    console.error(err);
    res.status(500).send("AI/TTS failed");
  }
});

app.use(express.raw({ type: "audio/wav", limit: "10mb" }));

app.post("/upload-audio", async (req, res) => {
  try {
    fs.writeFileSync("input.wav", req.body);
    console.log("Saved input.wav:", req.body.length, "bytes");

    const question = await transcribeAudio();
    console.log("User said:", question);

    const reply = await askOllama(question);
    console.log("Kiwi:", reply);

    await makeSpeech(reply);

    res.send(reply);
  } catch (err) {
    console.error(err);
    res.status(500).send("Voice pipeline failed");
  }
});

app.listen(PORT, "0.0.0.0", () => {
  console.log(`Kiwi server running on http://0.0.0.0:${PORT}`);
});