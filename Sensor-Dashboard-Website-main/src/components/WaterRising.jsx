import { motion } from "framer-motion";

export default function WaterRising({ waterLevel = 0, maxLevel = 1023 }) {
  const pct = Math.min(Math.max(waterLevel / maxLevel, 0), 1);
  const BASE_VH = 5;
  const heightVh = BASE_VH + pct * 20;

  return (
    <motion.div
      style={{
        position: "absolute",
        bottom: 0,
        left: 0,
        right: 0,
        zIndex: 5,
        overflow: "hidden",
      }}
      animate={{ height: `${Math.max(heightVh, 0)}vh` }}
      transition={{ type: "spring", damping: 30, stiffness: 25 }}
    >
      <svg
        viewBox="0 0 1440 80"
        preserveAspectRatio="none"
        style={{
          position: "absolute",
          top: -30,
          left: 0,
          width: "100%",
          height: 60,
        }}
      >
        <motion.path
          fill="rgba(56, 189, 248, 0.5)"
          animate={{
            d: [
              "M0,40 C240,10 480,70 720,40 C960,10 1200,70 1440,40 L1440,80 L0,80 Z",
              "M0,40 C240,70 480,10 720,40 C960,70 1200,10 1440,40 L1440,80 L0,80 Z",
              "M0,40 C240,10 480,70 720,40 C960,10 1200,70 1440,40 L1440,80 L0,80 Z",
            ],
          }}
          transition={{ duration: 3.5, repeat: Infinity, ease: "easeInOut" }}
        />
      </svg>
      <div
        style={{
          position: "absolute",
          top: 20,
          left: 0,
          right: 0,
          bottom: 0,
          background:
            "linear-gradient(to bottom, rgba(56,189,248,0.28), rgba(2,132,199,0.5))",
        }}
      />
    </motion.div>
  );
}
