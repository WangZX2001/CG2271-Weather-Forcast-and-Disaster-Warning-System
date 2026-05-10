import { useState } from "react";
import AutoAwesomeIcon from "@mui/icons-material/AutoAwesome";
import Dialog from "@mui/material/Dialog";
import DialogContent from "@mui/material/DialogContent";
import DialogTitle from "@mui/material/DialogTitle";
import Typography from "@mui/material/Typography";
import IconButton from "@mui/material/IconButton";
import CloseIcon from "@mui/icons-material/Close";
import CircularProgress from "@mui/material/CircularProgress";
import { collection, query, where, orderBy, getDocs } from "firebase/firestore";
import { db } from "../firebase";

const OPENAI_API_KEY = import.meta.env.VITE_OPENAI_API_KEY;

async function fetchLastHourReadings() {
  const since = Math.floor((Date.now() - 60 * 60 * 1000) / 1000);
  const q = query(
    collection(db, "readings"),
    where("timestamp", ">=", since),
    orderBy("timestamp", "asc")
  );
  const snap = await getDocs(q);
  return snap.docs.map((d) => {
    const { timestamp, ...fields } = d.data();
    return {
      time: timestamp
        ? new Date(timestamp * 1000).toLocaleTimeString()
        : "unknown",
      ...fields,
    };
  });
}

function buildPrompt(readings) {
  if (readings.length === 0)
    return "There are no sensor readings from the last hour. Let the user know there is no data to analyse.";

  const summary = readings
    .map(
      (r) =>
        `[${r.time}] temp=${r.temperature ?? "?"}°C  humidity=${
          r.humidity ?? "?"
        }%  water=${r.waterLevel ?? "?"}%  shock=${r.shock ?? "?"}  flame=${
          r.flame ?? "?"
        }`
    )
    .join("\n");

  return `You are analysing sensor data from an environmental monitoring dashboard.
Here are the readings from the last hour (${readings.length} samples):

${summary}

Give a concise insight summary in exactly 2 sentences. Highlight the most important trend or anomaly and reference specific values where relevant.`;
}

export default function GenerateInsights() {
  const [open, setOpen] = useState(false);
  const [result, setResult] = useState("");
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState("");

  const handleClick = async () => {
    setOpen(true);
    setResult("");
    setError("");
    setLoading(true);

    try {
      const readings = await fetchLastHourReadings();
      const prompt = buildPrompt(readings);

      const res = await fetch("https://api.openai.com/v1/chat/completions", {
        method: "POST",
        headers: {
          "Content-Type": "application/json",
          Authorization: `Bearer ${OPENAI_API_KEY}`,
        },
        body: JSON.stringify({
          model: "gpt-4o-mini",
          messages: [{ role: "user", content: prompt }],
          max_tokens: 300,
        }),
      });

      if (!res.ok) throw new Error(`OpenAI error: ${res.status}`);
      const json = await res.json();
      setResult(json.choices[0].message.content.trim());
    } catch (err) {
      setError(err.message);
    } finally {
      setLoading(false);
    }
  };

  return (
    <>
      <button
        onClick={handleClick}
        style={{
          display: "flex",
          alignItems: "center",
          gap: 6,
          padding: "6px 14px",
          cursor: "pointer",
          background: "rgba(99, 102, 241, 0.2)",
          border: "1px solid rgba(99, 102, 241, 0.4)",
          borderRadius: 16,
          backdropFilter: "blur(14px)",
          WebkitBackdropFilter: "blur(14px)",
          outline: "none",
          transition: "background 0.2s",
        }}
        onMouseEnter={(e) =>
          (e.currentTarget.style.background = "rgba(99, 102, 241, 0.35)")
        }
        onMouseLeave={(e) =>
          (e.currentTarget.style.background = "rgba(99, 102, 241, 0.2)")
        }
      >
        <AutoAwesomeIcon sx={{ fontSize: 16, color: "#a5b4fc" }} />
        <Typography
          variant="caption"
          sx={{
            color: "#a5b4fc",
            fontWeight: 600,
            fontSize: "0.75rem",
            letterSpacing: "0.05em",
            userSelect: "none",
          }}
        >
          Generate Insights
        </Typography>
      </button>

      <Dialog
        open={open}
        onClose={() => setOpen(false)}
        slotProps={{
          paper: {
            style: {
              background: "rgba(10, 15, 25, 0.92)",
              backdropFilter: "blur(20px)",
              border: "1px solid rgba(99, 102, 241, 0.3)",
              borderRadius: 16,
              minWidth: 360,
              maxWidth: 520,
            },
          },
        }}
      >
        <DialogTitle
          sx={{
            color: "#a5b4fc",
            fontWeight: 700,
            fontSize: "0.95rem",
            letterSpacing: "0.06em",
            pr: 6,
          }}
        >
          Last Hour Insights
          <IconButton
            onClick={() => setOpen(false)}
            size="small"
            sx={{ position: "absolute", right: 12, top: 10, color: "#475569" }}
          >
            <CloseIcon fontSize="small" />
          </IconButton>
        </DialogTitle>

        <DialogContent sx={{ pb: 3 }}>
          {loading && (
            <div
              style={{
                display: "flex",
                justifyContent: "center",
                padding: "16px 0",
              }}
            >
              <CircularProgress size={28} sx={{ color: "#a5b4fc" }} />
            </div>
          )}
          {result && (
            <Typography
              sx={{
                color: "#f1f5f9",
                fontSize: "0.95rem",
                lineHeight: 1.7,
              }}
            >
              {result}
            </Typography>
          )}
          {error && (
            <Typography sx={{ color: "#f87171", fontSize: "0.85rem" }}>
              {error}
            </Typography>
          )}
        </DialogContent>
      </Dialog>
    </>
  );
}
