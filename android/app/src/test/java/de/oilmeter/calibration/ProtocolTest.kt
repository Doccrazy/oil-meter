package de.oilmeter.calibration

import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Test

class ProtocolTest {
    @Test fun parsesMessages() {
        assertEquals(Message.Hello("oilmeter-cal", 1), parseLine("H,oilmeter-cal,1"))
        assertEquals(Message.Ping(7, 5832), parseLine("P,7,5832\r"))
        assertEquals(Message.Battery(3.91f), parseLine("B,3.910"))
    }

    @Test fun rejectsGarbage() {
        assertNull(parseLine(""))
        assertNull(parseLine("P,1"))
        assertNull(parseLine("P,x,y"))
        assertNull(parseLine("ESP-ROM:esp32c6-20220919"))
    }

    @Test fun distanceFromEcho() {
        // 20 degC: 343.42 m/s -> 100 cm needs a 5824 us round trip
        assertEquals(100f, distanceCm(5824, 20f), 0.1f)
    }

    @Test fun temperatureCorrection() {
        val measured = distanceCm(5824, 12f)
        val t = temperatureForDistance(measured, 100f, 12f)
        assertEquals(100f, distanceCm(5824, t), 0.01f)
    }

    @Test fun stats() {
        val s = statsOf(listOf(1f, 2f, 3f, 4f))!!
        assertEquals(2.5f, s.median, 1e-6f)
        assertEquals(2.5f, s.mean, 1e-6f)
        assertEquals(1.118f, s.stdDev, 1e-3f)
    }

    @Test fun lineAssemblerHandlesSplitChunks() {
        val lines = mutableListOf<String>()
        val a = LineAssembler { lines += it }
        a.feed("P,1,".toByteArray())
        a.feed("100\r\nP,2,".toByteArray())
        a.feed("200\n".toByteArray())
        assertEquals(listOf("P,1,100", "P,2,200"), lines)
    }
}
