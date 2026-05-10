import { useEffect, useState } from "react";
import { doc, onSnapshot } from "firebase/firestore";
import { db } from "./firebase";
import { createTheme, ThemeProvider, CssBaseline } from "@mui/material";
import Chip from "@mui/material/Chip";
import Typography from "@mui/material/Typography";
import DeviceThermostatIcon from "@mui/icons-material/DeviceThermostat";
import WaterDropIcon from "@mui/icons-material/WaterDrop";
import BoltIcon from "@mui/icons-material/Bolt";
import LocalFireDepartmentIcon from "@mui/icons-material/LocalFireDepartment";
import WavesIcon from "@mui/icons-material/Waves";
import FiberManualRecordIcon from "@mui/icons-material/FiberManualRecord";
import WarningAmberIcon from "@mui/icons-material/WarningAmber";
import CityBackground from "./components/CityBackground";
import SeismicOverlay from "./components/SeismicOverlay";
import RainCanvas from "./components/RainCanvas";
import WaterRising from "./components/WaterRising";
import Thermometer from "./components/Thermometer";
import GenerateInsights from "./components/GenerateInsights";

const darkTheme = createTheme({ palette: { mode: "dark" } });

const THRESHOLDS = {
  temperature: 35,
  humidity: 80,
  waterLevel: 83, // matches 5cm threshold set in the ESP
};

function isAlerted(key, value) {
  if (value === undefined || value === null) return false;
  if (key === "shock" || key === "flame") return Boolean(Number(value));
  return Number(value) > THRESHOLDS[key];
}

const COLUMN_1 = [
  {
    key: "temperature",
    label: "Temperature",
    unit: "°C",
    Icon: DeviceThermostatIcon,
    color: "#f97316",
  },
  {
    key: "humidity",
    label: "Humidity",
    unit: "%",
    Icon: WaterDropIcon,
    color: "#38bdf8",
  },
];
const COLUMN_2 = [
  { key: "shock", label: "Shock", unit: "", Icon: BoltIcon, color: "#facc15" },
  {
    key: "flame",
    label: "Flame",
    unit: "",
    Icon: LocalFireDepartmentIcon,
    color: "#ef4444",
  },
];
const COLUMN_3 = [
  {
    key: "waterLevel",
    label: "Water Level",
    unit: "%",
    Icon: WavesIcon,
    color: "#34d399",
  },
];

const CARD_H = "18vh";

const glass = {
  background: "rgba(10, 15, 25, 0.68)",
  backdropFilter: "blur(14px)",
  WebkitBackdropFilter: "blur(14px)",
  border: "1px solid rgba(255,255,255,0.08)",
  borderRadius: 16,
};

function SensorCard({ label, unit, Icon, color, value, alerted }) {
  return (
    <div
      style={{
        ...glass,
        height: CARD_H,
        width: "100%",
        display: "flex",
        flexDirection: "column",
        alignItems: "center",
        justifyContent: "center",
        gap: 6,
        padding: "10px 8px",
        position: "relative",
        overflow: "hidden",
        animation: alerted ? "alertBorder 0.8s infinite" : "none",
      }}
    >
      {alerted && (
        <div
          style={{
            position: "absolute",
            inset: 0,
            borderRadius: 16,
            animation: "alertFlash 0.8s infinite",
            pointerEvents: "none",
            zIndex: 0,
          }}
        />
      )}

      <div
        style={{
          position: "relative",
          zIndex: 1,
          display: "flex",
          flexDirection: "column",
          alignItems: "center",
          gap: 8,
          width: "100%",
        }}
      >
        {/* Alert badge */}
        {alerted && (
          <div
            style={{
              position: "absolute",
              top: -8,
              right: 10,
              display: "flex",
              alignItems: "center",
              gap: 4,
            }}
          >
            <WarningAmberIcon sx={{ fontSize: 30, color: "#f59e0b" }} />
            <Typography
              variant="caption"
              sx={{
                fontSize: "0.8rem",
                color: "#f59e0b",
                fontWeight: 700,
                letterSpacing: "0.06em",
              }}
            >
              ALERT
            </Typography>
          </div>
        )}

        <Icon sx={{ fontSize: 36, color: alerted ? "#ef4444" : color }} />

        <Typography
          variant="caption"
          sx={{
            color: "#475569",
            textTransform: "uppercase",
            letterSpacing: "0.08em",
            fontSize: "0.9rem",
          }}
        >
          {label}
        </Typography>

        <Typography
          variant="h5"
          fontWeight={700}
          sx={{ color: alerted ? "#ef4444" : "#f1f5f9", lineHeight: 1 }}
        >
          {value !== undefined && value !== null ? (
            <>
              {String(value)}
              {unit && (
                <Typography
                  component="span"
                  variant="body2"
                  sx={{ color: "#64748b", ml: 0.5 }}
                >
                  {unit}
                </Typography>
              )}
            </>
          ) : (
            <Typography component="span" sx={{ color: "#334155" }}>
              —
            </Typography>
          )}
        </Typography>
      </div>
    </div>
  );
}

export default function App() {
  const [data, setData] = useState(null);
  const [lastUpdated, setLastUpdated] = useState(null);
  const [error, setError] = useState(null);

  useEffect(() => {
    const unsub = onSnapshot(
      doc(db, "sensor", "latest"),
      (snap) => {
        if (!snap.exists()) return;
        setData(snap.data());
        setLastUpdated(new Date());
      },
      (err) => {
        console.error(err);
        setError(err.message);
      }
    );
    return unsub;
  }, []);

  const thermoH = `calc(2 * ${CARD_H} + 10px)`;

  return (
    <ThemeProvider theme={darkTheme}>
      <CssBaseline />

      <div
        style={{
          position: "relative",
          width: "100vw",
          height: "100vh",
          overflow: "hidden",
        }}
      >
        <CityBackground flame={Number(data?.flame ?? 0)} />
        <RainCanvas humidity={Number(data?.humidity ?? 0)} />
        <WaterRising waterLevel={Number(data?.waterLevel ?? 0)} maxLevel={100} />
        <div
          style={{
            position: "absolute",
            inset: 0,
            zIndex: 10,
            display: "flex",
            flexDirection: "column",
            pointerEvents: "none",
          }}
        >
          {/* Header */}
          <div
            style={{
              display: "flex",
              alignItems: "center",
              gap: 12,
              padding: "1.1rem 1.25rem",
              pointerEvents: "auto",
              flexShrink: 0,
            }}
          >
            <Typography
              variant="h5"
              fontWeight={700}
              sx={{ color: "white", textShadow: "0 2px 12px rgba(0,0,0,0.9)" }}
            >
              City Sensor Dashboard
            </Typography>
            <Chip
              icon={
                <FiberManualRecordIcon
                  sx={{
                    fontSize: "10px !important",
                    color: data ? "#4ade80" : "#94a3b8",
                  }}
                />
              }
              label={data ? "Live" : "Waiting…"}
              size="small"
              sx={{
                bgcolor: data ? "rgba(20,83,45,0.75)" : "rgba(30,41,59,0.75)",
                color: data ? "#4ade80" : "#94a3b8",
                fontWeight: 600,
                fontSize: "0.7rem",
                letterSpacing: "0.05em",
                backdropFilter: "blur(8px)",
              }}
            />
            {lastUpdated && (
              <Typography
                variant="caption"
                sx={{ color: "rgba(100,116,139,0.8)", ml: "auto" }}
              >
                Last updated: {lastUpdated.toLocaleTimeString()}
              </Typography>
            )}
            <div
              style={{
                display: "flex",
                alignItems: "center",
                gap: 8,
                marginLeft: "auto",
              }}
            >
              <GenerateInsights />
            </div>
          </div>

          <div
            style={{
              display: "flex",
              alignItems: "flex-start",
              gap: 10,
              padding: "0 1.25rem",
              pointerEvents: "auto",
              flexShrink: 0,
            }}
          >
            {/* Thermometer */}
            <div
              style={{
                ...glass,
                height: thermoH,
                padding: "16px 12px",
                display: "flex",
                flexDirection: "column",
                alignItems: "center",
                justifyContent: "center",
                width: 90,
                flexShrink: 0,
              }}
            >
              <Thermometer temperature={data?.temperature} />
            </div>

            <div
              style={{
                display: "flex",
                flexDirection: "column",
                gap: 10,
                flex: 1,
              }}
            >
              {COLUMN_1.map(({ key, label, unit, Icon, color }) => (
                <SensorCard
                  key={key}
                  label={label}
                  unit={unit}
                  Icon={Icon}
                  color={color}
                  value={data?.[key]}
                  alerted={isAlerted(key, data?.[key])}
                />
              ))}
            </div>

            <div
              style={{
                display: "flex",
                flexDirection: "column",
                gap: 10,
                flex: 1,
              }}
            >
              {COLUMN_2.map(({ key, label, unit, Icon, color }) => (
                <SensorCard
                  key={key}
                  label={label}
                  unit={unit}
                  Icon={Icon}
                  color={color}
                  value={data?.[key]}
                  alerted={isAlerted(key, data?.[key])}
                />
              ))}
            </div>

            <div
              style={{
                display: "flex",
                flexDirection: "column",
                gap: 10,
                flex: 1,
              }}
            >
              {COLUMN_3.map(({ key, label, unit, Icon, color }) => (
                <SensorCard
                  key={key}
                  label={label}
                  unit={unit}
                  Icon={Icon}
                  color={color}
                  value={data?.[key]}
                  alerted={isAlerted(key, data?.[key])}
                />
              ))}
            </div>
          </div>

        </div>

        {/* Error toast */}
        {error && (
          <div
            style={{
              position: "absolute",
              top: 80,
              left: "50%",
              transform: "translateX(-50%)",
              zIndex: 30,
              ...glass,
              background: "rgba(69,10,10,0.85)",
              color: "#fca5a5",
              border: "1px solid #7f1d1d",
              padding: "8px 16px",
              fontSize: "0.875rem",
            }}
          >
            Firebase error: {error}
          </div>
        )}
        <SeismicOverlay active={Boolean(Number(data?.shock ?? 0))} />
      </div>
    </ThemeProvider>
  );
}
