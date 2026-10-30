package de.oilmeter.calibration

import kotlin.math.sqrt

/** One parsed line of the calibration firmware (see calibration/main.cpp). */
sealed interface Message {
    data class Hello(val id: String, val version: Int) : Message
    /** [echoUs] is the raw round trip time, 0 when no echo came back. */
    data class Ping(val seq: Long, val echoUs: Long) : Message
    data class Battery(val volts: Float) : Message
}

fun parseLine(line: String): Message? {
    val f = line.trim().split(',')
    return try {
        when (f[0]) {
            "H" -> Message.Hello(f[1], f[2].toInt())
            "P" -> Message.Ping(f[1].toLong(), f[2].toLong())
            "B" -> Message.Battery(f[1].toFloat())
            else -> null
        }
    } catch (e: IndexOutOfBoundsException) {
        null
    } catch (e: NumberFormatException) {
        null
    }
}

/** Same speed of sound model as the firmware (src/sensor.cpp). */
fun speedOfSoundCmPerUs(temperatureC: Float): Float = (331.3f + 0.606f * temperatureC) / 10000f

fun distanceCm(echoUs: Long, temperatureC: Float): Float = echoUs * speedOfSoundCmPerUs(temperatureC) / 2f

/** Air temperature at which [measuredCm] (taken at [assumedC]) would equal [trueCm]. */
fun temperatureForDistance(measuredCm: Float, trueCm: Float, assumedC: Float): Float {
    val v = (331.3f + 0.606f * assumedC) * trueCm / measuredCm
    return (v - 331.3f) / 0.606f
}

data class Stats(val count: Int, val median: Float, val mean: Float, val stdDev: Float, val min: Float, val max: Float)

fun statsOf(values: List<Float>): Stats? {
    if (values.isEmpty()) return null
    val sorted = values.sorted()
    val n = sorted.size
    val median = if (n % 2 == 1) sorted[n / 2] else (sorted[n / 2 - 1] + sorted[n / 2]) / 2f
    val mean = values.sum() / n
    val variance = values.sumOf { ((it - mean) * (it - mean)).toDouble() } / n
    return Stats(n, median, mean, sqrt(variance).toFloat(), sorted.first(), sorted.last())
}

/** Splits a byte stream into text lines. */
class LineAssembler(private val onLine: (String) -> Unit) {
    private val buf = StringBuilder()

    fun feed(bytes: ByteArray) {
        for (b in bytes) {
            when (val c = b.toInt().toChar()) {
                '\n' -> {
                    onLine(buf.toString())
                    buf.setLength(0)
                }
                '\r' -> {}
                else -> if (buf.length < 256) buf.append(c)
            }
        }
    }
}
