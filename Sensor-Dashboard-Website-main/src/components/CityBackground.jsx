
const SKY_TOP = '#1a1040'
const SKY_MID = '#3d1f6e'
const SKY_BOT = '#c0562a'

const FAR = [
  [0,70,80],[80,50,60],[140,90,100],[240,60,70],[310,80,90],[400,55,65],
  [465,85,80],[560,65,95],[635,100,110],[745,55,75],[810,80,95],[900,90,85],
  [1000,65,70],[1075,85,100],[1170,60,80],[1240,80,70],[1330,70,90],[1410,80,80],
]
const MID = [
  [0,85,140],[95,70,180],[175,60,160],[245,95,230],[350,75,170],[435,65,200],
  [510,105,185],[625,85,240],[720,80,195],[810,95,165],[915,75,200],[1000,85,175],
  [1095,100,220],[1205,80,165],[1295,85,185],[1390,100,150],
]
const CLOSE = [
  [0,110,200],[120,85,270],[215,95,235],[320,80,310],[410,105,250],[525,85,295],
  [620,125,330],[755,95,255],[860,80,285],[950,105,300],[1065,85,260],[1160,100,290],
  [1270,115,240],[1395,90,220],
]

const SVG_H = 500
const FIRE_POSITIONS = [
  { x: 162,  y: SVG_H - 270 }, // building [120,85,270]
  { x: 462,  y: SVG_H - 250 }, // building [410,105,250]
  { x: 682,  y: SVG_H - 330 }, // building [620,125,330]
  { x: 1002, y: SVG_H - 300 }, // building [950,105,300]
  { x: 1210, y: SVG_H - 290 }, // building [1160,100,290]
]

function Flame({ x, y, seed = 0 }) {
  const d1 = 0.4 + seed * 0.05
  const d2 = 0.35 + seed * 0.04
  const d3 = 0.28 + seed * 0.03
  return (
    <g transform={`translate(${x}, ${y})`}>
      {/* Outer flame */}
      <ellipse cx="0" cy="-12" rx="9" ry="22" fill="#f97316" opacity="0.85">
        <animate attributeName="ry" values="22;17;24;19;22" dur={`${d1}s`} repeatCount="indefinite"/>
        <animate attributeName="rx" values="9;7;10;8;9" dur={`${d2}s`} repeatCount="indefinite"/>
        <animate attributeName="opacity" values="0.85;0.65;0.9;0.75;0.85" dur={`${d3}s`} repeatCount="indefinite"/>
      </ellipse>
      {/* Mid flame */}
      <ellipse cx="0" cy="-6" rx="6" ry="15" fill="#fbbf24" opacity="0.9">
        <animate attributeName="ry" values="15;12;17;13;15" dur={`${d2}s`} repeatCount="indefinite"/>
        <animate attributeName="rx" values="6;5;7;6;6" dur={`${d3}s`} repeatCount="indefinite"/>
      </ellipse>
      {/* Core */}
      <ellipse cx="0" cy="0" rx="3" ry="8" fill="#fef9c3" opacity="0.95">
        <animate attributeName="ry" values="8;6;9;7;8" dur={`${d3}s`} repeatCount="indefinite"/>
      </ellipse>
    </g>
  )
}

function Windows({ buildings, svgH }) {
  return buildings.flatMap(([bx, bw, bh], bi) => {
    const cols = Math.max(1, Math.floor(bw / 16))
    const rows = Math.max(1, Math.floor(bh / 22))
    const colStep = (bw - 10) / cols
    const rowStep = (bh - 20) / rows
    const by = svgH - bh
    const wins = []
    for (let c = 0; c < cols; c++) {
      for (let r = 0; r < rows; r++) {
        if ((bi * 11 + c * 7 + r * 3) % 4 === 0) continue
        wins.push(
          <rect
            key={`${bi}-${c}-${r}`}
            x={bx + 5 + c * colStep}
            y={by + 10 + r * rowStep}
            width={7} height={8} rx={1}
            fill="rgba(255,220,140,0.55)"
          />
        )
      }
    }
    return wins
  })
}

export default function CityBackground({ flame }) {
  const flameActive = Number(flame ?? 0) > 0

  return (
    <div style={{ position: 'absolute', inset: 0, zIndex: 0 }}>
      <div
        style={{
          position: 'absolute', inset: 0,
          background: `linear-gradient(to bottom, ${SKY_TOP} 0%, ${SKY_MID} 55%, ${SKY_BOT} 100%)`,
        }}
      />

      {/* Stars */}
      {Array.from({ length: 70 }, (_, i) => (
        <div
          key={i}
          style={{
            position: 'absolute',
            width: i % 5 === 0 ? 2 : 1.5,
            height: i % 5 === 0 ? 2 : 1.5,
            borderRadius: '50%',
            background: 'white',
            opacity: 0.3 + (i % 4) * 0.15,
            left: `${(i * 137.508) % 100}%`,
            top: `${(i * 97.31) % 50}%`,
            animation: `twinkle ${1.5 + (i % 3) * 0.8}s ${(i % 5) * 0.4}s infinite alternate`,
          }}
        />
      ))}

      <svg
        viewBox={`0 0 1440 ${SVG_H}`}
        preserveAspectRatio="xMidYMax meet"
        style={{ position: 'absolute', bottom: 0, left: 0, width: '100%', height: '75%' }}
      >
        <g opacity="0.35">
          {FAR.map(([x, w, h], i) => (
            <rect key={i} x={x} y={SVG_H - h} width={w} height={h} fill="#2a4a6a" />
          ))}
        </g>

        <g opacity="0.65">
          {MID.map(([x, w, h], i) => (
            <rect key={i} x={x} y={SVG_H - h} width={w} height={h} fill="#0e1e30" />
          ))}
          <g opacity="0.5"><Windows buildings={MID} svgH={SVG_H} /></g>
        </g>

        <g>
          {CLOSE.map(([x, w, h], i) => (
            <rect key={i} x={x} y={SVG_H - h} width={w} height={h} fill="#07101a" />
          ))}
          <Windows buildings={CLOSE} svgH={SVG_H} />
        </g>

        {flameActive && FIRE_POSITIONS.map((pos, i) => (
          <Flame key={i} x={pos.x} y={pos.y} seed={i} />
        ))}

        <rect x="0" y={SVG_H - 4} width="1440" height="4" fill="#040a10" />
      </svg>
    </div>
  )
}
