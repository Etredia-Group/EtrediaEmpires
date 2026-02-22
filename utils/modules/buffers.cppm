export module utils:buffers;
import std;

export namespace ee_utils {
    /**
     * Reads the next readSize bytes from the source buffer into the destination buffer
     * @param dst pointer to destination buffer. Data will be copied to this pointer.
     * @param srcPointer pointer to the current position in the source bytes buffer.
     * Data will be copied from this position and the pointer will be advanced to the end of the read data.
     * @param srcLeftoverSize number of bytes remaining in the source buffer starting from srcPointer
     * It will be reduced by the number of copied bytes..
     * @param readSize amount of bytes to copy.
     * @return true on success; false if readSize exceeds the number of remaining bytes in the source buffer.
     */
    bool readNext(void *dst, std::uint8_t * & srcPointer, std::size_t& srcLeftoverSize, std::size_t readSize);
    /**
     * Writes the writeSize bytes from the source buffet into the next position in destination buffer
     * @param dstPointer pointer to the current position in the destination bytes buffer.
     * Data will be copied to this position of buffer and pointer will be advanced to the end of the written data.
     * @param src pointer to source buffer. Data will be copied from this pointer.
     * @param dstLeftoverSize amount of bytes from dstPointer to the end of destination buffer.
     * It will be reduced by the number of copied bytes..
     * @param writeSize amount of bytes to copy.
     * @return true on success; false if writeSize exceeds the number of remaining bytes in the destination buffer.
     */
    bool writeNext(std::uint8_t * & dstPointer, void *src, std::size_t& dstLeftoverSize, std::size_t writeSize);
}
