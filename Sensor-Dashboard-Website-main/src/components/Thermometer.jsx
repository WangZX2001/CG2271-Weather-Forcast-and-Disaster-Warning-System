import { motion } from 'framer-motion'

const MIN_TEMP = -10
const MAX_TEMP = 50
const TUBE_H = 100

function tempColor(temp) {
  if (temp < 0)  return '#60a5fa'
  if (temp < 15) return '#34d399'
  if (temp < 28) return '#fbbf24'
  if (temp < 35) return '#f97316'
  return               '#ef4444'
}

export default function Thermometer({ temperature }) {
  const temp = temperature ?? 22
  const pct = Math.min(Math.max((temp - MIN_TEMP) / (MAX_TEMP - MIN_TEMP), 0), 1)
  const fillH = pct * TUBE_H
  const color = tempColor(temp)
  const TUBE_TOP = 18
  const TUBE_BOT = TUBE_TOP + TUBE_H

  return (
    <div style={{ display: 'flex', flexDirection: 'column', alignItems: 'center', gap: 2, userSelect: 'none' }}>
      <span style={{ fontSize: 10, color: '#64748b', fontFamily: 'monospace' }}>{MAX_TEMP}°</span>
      <svg width="36" height="158" viewBox="0 0 36 158">
        {/* Tube outline */}
        <rect x="13" y={TUBE_TOP} width="10" height={TUBE_H} rx="5"
          fill="rgba(255,255,255,0.06)" stroke="rgba(255,255,255,0.15)" strokeWidth="1" />
        {/* Tick marks */}
        {[0, 25, 50, 75, 100].map(t => (
          <line key={t} x1="24" y1={TUBE_TOP + t} x2="28" y2={TUBE_TOP + t}
            stroke="rgba(255,255,255,0.2)" strokeWidth="1" />
        ))}
        {/* Fill */}
        <motion.rect
          x="13" width="10" rx="5"
          animate={{ height: fillH, y: TUBE_BOT - fillH, fill: color }}
          transition={{ type: 'spring', damping: 20, stiffness: 40 }}
        />
        {/* Bulb */}
        <motion.circle cx="18" cy={TUBE_BOT + 14} r="12"
          animate={{ fill: color }} transition={{ duration: 0.5 }} />
        {/* Bulb shine */}
        <circle cx="22" cy={TUBE_BOT + 10} r="3.5" fill="rgba(255,255,255,0.22)" />
      </svg>
      <span style={{ fontSize: 10, color: '#64748b', fontFamily: 'monospace' }}>{MIN_TEMP}°</span>
      <motion.span
        style={{ fontSize: 14, fontWeight: 700, marginTop: 4 }}
        animate={{ color }}
        transition={{ duration: 0.5 }}
      >
        {typeof temp === 'number' ? temp.toFixed(1) : temp}°C
      </motion.span>
    </div>
  )
}
