import { useEffect, useRef } from "react";
import { motion, AnimatePresence } from "framer-motion";
import { Typography } from "@mui/material";

function SeismicCanvas() {
  const ref = useRef(null);

  useEffect(() => {
    const canvas = ref.current;
    const ctx = canvas.getContext("2d");
    let offset = 0;
    let id;

    const draw = () => {
      const W = canvas.width;
      const H = canvas.height;
      ctx.clearRect(0, 0, W, H);

      ctx.beginPath();
      ctx.strokeStyle = "#facc15";
      ctx.lineWidth = 1.5;
      ctx.shadowColor = "#facc15";
      ctx.shadowBlur = 6;

      for (let x = 0; x < W; x++) {
        const t = (x + offset) * 0.04;
        const y =
          H / 2 +
          Math.sin(t) * 10 +
          Math.sin(t * 2.3 + 1) * 6 +
          Math.sin(t * 5.1 + 2) * 3;
        if (x === 0) ctx.moveTo(x, y);
        else ctx.lineTo(x, y);
      }
      ctx.stroke();
      offset += 2;
      id = requestAnimationFrame(draw);
    };

    draw();
    return () => cancelAnimationFrame(id);
  }, []);

  return (
    <canvas
      ref={ref}
      width={760}
      height={48}
      style={{ width: "100%", height: 48, display: "block" }}
    />
  );
}

export default function SeismicOverlay({ active }) {
  return (
    <AnimatePresence>
      {active && (
        <motion.div
          style={{
            position: "absolute",
            inset: 0,
            zIndex: 15,
            pointerEvents: "none",
          }}
          initial={{ opacity: 0 }}
          animate={{ opacity: 1 }}
          exit={{ opacity: 0 }}
          transition={{ duration: 0.4 }}
        >
          <div
            style={{
              position: "absolute",
              inset: 0,
              boxShadow: "inset 0 0 80px 20px rgba(220,38,38,0.35)",
              borderRadius: 0,
            }}
          />

          <div
            style={{
              position: "absolute",
              bottom: 0,
              left: 0,
              right: 0,
              background: "rgba(10,15,25,0.75)",
              backdropFilter: "blur(6px)",
              borderTop: "1px solid rgba(250,204,21,0.3)",
              padding: "4px 12px",
              display: "flex",
              alignItems: "center",
              gap: 10,
            }}
          >
            <Typography
              component="span"
              style={{
                fontSize: "0.6rem",
                fontWeight: 700,
                color: "#facc15",
                letterSpacing: "0.1em",
                textTransform: "uppercase",
                whiteSpace: "nowrap",
                fontFamily: "system-ui",
              }}
            >
              ⚠ Seismic Activity
            </Typography>
            <div style={{ flex: 1, overflow: "hidden" }}>
              <SeismicCanvas />
            </div>
          </div>
        </motion.div>
      )}
    </AnimatePresence>
  );
}
