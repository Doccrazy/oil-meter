package de.oilmeter.calibration

import android.app.Activity
import android.app.PendingIntent
import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.graphics.Typeface
import android.hardware.usb.UsbManager
import android.os.Build
import android.os.Bundle
import android.text.InputType
import android.view.Gravity
import android.view.ViewGroup
import android.view.WindowManager
import android.widget.EditText
import android.widget.LinearLayout
import android.widget.ScrollView
import android.widget.TextView
import com.hoho.android.usbserial.driver.UsbSerialPort
import com.hoho.android.usbserial.driver.UsbSerialProber
import com.hoho.android.usbserial.util.SerialInputOutputManager

/**
 * Live view of the oil meter's calibration firmware over USB (phone = USB host).
 * Distance shown is the median of the last 9 valid pings, like the production firmware.
 */
class MainActivity : Activity(), SerialInputOutputManager.Listener {
    private lateinit var status: TextView
    private lateinit var big: TextView
    private lateinit var sub: TextView
    private lateinit var plot: PlotView
    private lateinit var statsText: TextView
    private lateinit var calText: TextView
    private lateinit var tempInput: EditText
    private lateinit var refInput: EditText

    private var port: UsbSerialPort? = null
    private var io: SerialInputOutputManager? = null
    private val lines = LineAssembler { line -> runOnUiThread { handle(line) } }

    private val plotValues = ArrayDeque<Float>()  // NaN = no echo / out of range
    private val validValues = ArrayDeque<Float>()  // temperature compensated, valid pings only
    private var lastEchoUs = 0L
    private var batteryV: Float? = null
    private var pings = 0L
    private var timeouts = 0L

    private val permissionReceiver = object : BroadcastReceiver() {
        override fun onReceive(context: Context, intent: Intent) {
            if (intent.getBooleanExtra(UsbManager.EXTRA_PERMISSION_GRANTED, false)) connect()
            else status.text = "USB permission denied"
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
        buildUi()
        val filter = IntentFilter(ACTION_USB_PERMISSION)
        if (Build.VERSION.SDK_INT >= 33) registerReceiver(permissionReceiver, filter, RECEIVER_NOT_EXPORTED)
        else registerReceiver(permissionReceiver, filter)
    }

    override fun onResume() {
        super.onResume()
        if (port == null) connect()
    }

    override fun onNewIntent(intent: Intent) {
        super.onNewIntent(intent)
        if (port == null) connect()
    }

    override fun onDestroy() {
        unregisterReceiver(permissionReceiver)
        disconnect()
        super.onDestroy()
    }

    // ---- USB -------------------------------------------------------------------------

    private fun connect() {
        val usb = getSystemService(USB_SERVICE) as UsbManager
        val driver = UsbSerialProber.getDefaultProber().findAllDrivers(usb).firstOrNull()
        if (driver == null) {
            status.text = "No device. Plug the XIAO (calibration firmware) into the phone's USB port."
            return
        }
        if (!usb.hasPermission(driver.device)) {
            status.text = "Waiting for USB permission..."
            val pi = PendingIntent.getBroadcast(
                this, 0, Intent(ACTION_USB_PERMISSION).setPackage(packageName), PendingIntent.FLAG_MUTABLE
            )
            usb.requestPermission(driver.device, pi)
            return
        }
        val connection = usb.openDevice(driver.device)
        if (connection == null) {
            status.text = "Could not open the USB device"
            return
        }
        try {
            val p = driver.ports[0]
            p.open(connection)
            p.setParameters(115200, 8, UsbSerialPort.STOPBITS_1, UsbSerialPort.PARITY_NONE)
            p.dtr = true
            p.rts = true
            port = p
            io = SerialInputOutputManager(p, this).also { it.start() }
            status.text = "Connected, waiting for data..."
        } catch (e: Exception) {
            status.text = "USB error: ${e.message}"
            disconnect()
        }
    }

    private fun disconnect() {
        io?.listener = null
        io?.stop()
        io = null
        try {
            port?.close()
        } catch (_: Exception) {
        }
        port = null
    }

    override fun onNewData(data: ByteArray) = lines.feed(data)

    override fun onRunError(e: Exception) {
        runOnUiThread {
            disconnect()
            status.text = "Disconnected (${e.message}). Replug the sensor."
        }
    }

    // ---- Data ------------------------------------------------------------------------

    private fun handle(line: String) {
        when (val m = parseLine(line) ?: return) {
            is Message.Hello -> status.text = "Connected: ${m.id} v${m.version}"
            is Message.Battery -> batteryV = m.volts
            is Message.Ping -> onPing(m)
        }
    }

    private fun onPing(p: Message.Ping) {
        val d = if (p.echoUs > 0) distanceCm(p.echoUs, temperature()) else Float.NaN
        val valid = !d.isNaN() && d in MIN_CM..MAX_CM
        pings++
        if (p.echoUs == 0L) timeouts++
        lastEchoUs = p.echoUs
        plotValues.addLast(if (valid) d else Float.NaN)
        if (plotValues.size > PlotView.MAX_POINTS) plotValues.removeFirst()
        if (valid) {
            validValues.addLast(d)
            if (validValues.size > STATS_WINDOW) validValues.removeFirst()
        }
        render()
    }

    private fun temperature() = tempInput.text.toString().toFloatOrNull() ?: DEFAULT_TEMP_C

    private fun render() {
        val live = statsOf(validValues.takeLast(9))
        big.text = live?.let { "%.1f cm".format(it.median) } ?: "-- cm"
        sub.text = buildString {
            append("echo ").append(if (lastEchoUs > 0) "$lastEchoUs µs" else "none")
            append("   no-echo ${timeouts * 100 / maxOf(pings, 1)} %")
            batteryV?.let { append("   battery %.2f V".format(it)) }
        }
        plot.values = plotValues.toList()

        val s = statsOf(validValues)
        statsText.text = if (s == null) "" else
            "last ${s.count} valid pings\n" +
                "median %.2f cm   mean %.2f cm\n".format(s.median, s.mean) +
                "σ %.2f cm   min %.1f   max %.1f".format(s.stdDev, s.min, s.max)

        val ref = refInput.text.toString().toFloatOrNull()
        calText.text = when {
            s == null || ref == null -> "Enter the tape-measured distance from the probe face to the surface."
            else -> {
                val err = s.median - ref
                val t = temperatureForDistance(s.median, ref, temperature())
                "error %+.2f cm (%+.1f %%)\n".format(err, err / ref * 100) +
                    "→ SENSOR_OFFSET_CM: add %+.2f if the error is constant\n".format(-err) +
                    "→ AIR_TEMPERATURE_C: %.1f if it grows with distance".format(t)
            }
        }
    }

    // ---- UI --------------------------------------------------------------------------

    private fun dp(v: Int) = (v * resources.displayMetrics.density).toInt()

    private fun text(size: Float, bold: Boolean = false, mono: Boolean = false) = TextView(this).apply {
        textSize = size
        if (bold) setTypeface(null, Typeface.BOLD)
        if (mono) typeface = Typeface.MONOSPACE
    }

    private fun numberInput(hint: String, value: String) = EditText(this).apply {
        this.hint = hint
        setText(value)
        inputType = InputType.TYPE_CLASS_NUMBER or InputType.TYPE_NUMBER_FLAG_DECIMAL or InputType.TYPE_NUMBER_FLAG_SIGNED
        layoutParams = LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f)
    }

    private fun buildUi() {
        val root = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(dp(16), dp(16), dp(16), dp(16))
        }
        status = text(14f)
        big = text(56f, bold = true).apply { gravity = Gravity.CENTER }
        sub = text(14f).apply { gravity = Gravity.CENTER }
        plot = PlotView(this)
        statsText = text(14f, mono = true)
        calText = text(14f)
        tempInput = numberInput("Air temp °C", DEFAULT_TEMP_C.toString())
        refInput = numberInput("Tape measure cm", "")

        val inputs = LinearLayout(this).apply {
            addView(tempInput)
            addView(refInput)
        }
        root.addView(status)
        root.addView(big)
        root.addView(sub)
        root.addView(plot, LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, dp(160)).apply {
            topMargin = dp(12)
            bottomMargin = dp(12)
        })
        root.addView(statsText)
        root.addView(inputs)
        root.addView(calText)
        setContentView(ScrollView(this).apply { addView(root) })
        status.text = "Starting..."
    }

    companion object {
        private const val ACTION_USB_PERMISSION = "de.oilmeter.calibration.USB_PERMISSION"
        private const val DEFAULT_TEMP_C = 12f  // cfg::AIR_TEMPERATURE_C
        private const val MIN_CM = 20f  // cfg::SENSOR_MIN_DISTANCE_CM
        private const val MAX_CM = 450f  // cfg::SENSOR_MAX_DISTANCE_CM
        private const val STATS_WINDOW = 50
    }
}
