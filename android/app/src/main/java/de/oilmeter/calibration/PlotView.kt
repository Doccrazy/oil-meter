package de.oilmeter.calibration

import android.content.Context
import android.graphics.Canvas
import android.graphics.Color
import android.graphics.Paint
import android.graphics.Path
import android.view.View

/** Minimal strip chart of the most recent distances; NaN entries (no echo) leave gaps. */
class PlotView(context: Context) : View(context) {
    var values: List<Float> = emptyList()
        set(v) {
            field = v
            invalidate()
        }

    private val density = resources.displayMetrics.density
    private val line = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        color = Color.rgb(0x1a, 0x73, 0xe8)
        style = Paint.Style.STROKE
        strokeWidth = 2f * density
    }
    private val label = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        color = Color.GRAY
        textSize = 11f * density
    }
    private val frame = Paint().apply {
        color = Color.LTGRAY
        style = Paint.Style.STROKE
    }

    override fun onDraw(canvas: Canvas) {
        val w = width.toFloat()
        val h = height.toFloat()
        canvas.drawRect(0f, 0f, w, h, frame)
        val finite = values.filter { !it.isNaN() }
        if (finite.isEmpty()) return

        var lo = finite.min()
        var hi = finite.max()
        if (hi - lo < 1f) {  // keep a sensible scale when the signal is rock steady
            val mid = (hi + lo) / 2
            lo = mid - 0.5f
            hi = mid + 0.5f
        }
        val pad = 14f * density
        fun y(v: Float) = pad + (hi - v) / (hi - lo) * (h - 2 * pad)
        val dx = w / (MAX_POINTS - 1)
        val x0 = w - (values.size - 1) * dx

        val path = Path()
        var pen = false
        values.forEachIndexed { i, v ->
            if (v.isNaN()) {
                pen = false
            } else {
                val x = x0 + i * dx
                if (pen) path.lineTo(x, y(v)) else path.moveTo(x, y(v))
                pen = true
            }
        }
        canvas.drawPath(path, line)
        canvas.drawText("%.1f".format(hi), 4f * density, 11f * density, label)
        canvas.drawText("%.1f".format(lo), 4f * density, h - 3f * density, label)
    }

    companion object {
        const val MAX_POINTS = 150
    }
}
