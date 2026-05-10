import { useEffect, useRef } from 'react'

export default function RainCanvas({ humidity = 0 }) {
  const canvasRef = useRef(null)
  const humRef = useRef(humidity)

  useEffect(() => { humRef.current = humidity }, [humidity])

  useEffect(() => {
    const canvas = canvasRef.current
    const ctx = canvas.getContext('2d')
    const drops = []
    let animId

    const resize = () => {
      canvas.width = window.innerWidth
      canvas.height = window.innerHeight
    }
    resize()
    window.addEventListener('resize', resize)

    const makeDrop = () => ({
      x: Math.random() * canvas.width,
      y: Math.random() * canvas.height,
      len: 10 + Math.random() * 15,
      speed: 7 + Math.random() * 10,
      alpha: 0.1 + Math.random() * 0.3,
    })

    function draw() {
      ctx.clearRect(0, 0, canvas.width, canvas.height)

      const target = Math.floor((humRef.current / 100) * 400)
      while (drops.length < target) drops.push(makeDrop())
      drops.length = Math.min(drops.length, target)

      ctx.lineWidth = 1
      ctx.strokeStyle = '#b8d4f0'
      for (const d of drops) {
        d.y += d.speed
        if (d.y > canvas.height + d.len) {
          d.y = -d.len
          d.x = Math.random() * canvas.width
        }
        ctx.globalAlpha = d.alpha
        ctx.beginPath()
        ctx.moveTo(d.x, d.y)
        ctx.lineTo(d.x + 1.5, d.y + d.len)
        ctx.stroke()
      }
      ctx.globalAlpha = 1
      animId = requestAnimationFrame(draw)
    }

    draw()
    return () => {
      cancelAnimationFrame(animId)
      window.removeEventListener('resize', resize)
    }
  }, [])

  return (
    <canvas
      ref={canvasRef}
      style={{ position: 'absolute', inset: 0, pointerEvents: 'none', zIndex: 6 }}
    />
  )
}
