(defun setRemote (msg) {
(set-remote-state (bufget-f32 msg 0 'little-endian) (bufget-f32 msg 4 'little-endian) (bufget-u8 msg 8) (bufget-u8 msg 9) (bufget-u8 msg 10))
;(print (get-remote-state))
})

(loopwhile-thd 100 t {
        (def msg (canmsg-recv 0 -1))
        (def msgLen (- (buflen msg) 2))
        (def crc (crc16 msg msgLen))
        (if (eq crc (bufget-u16 msg msgLen)) (setRemote msg))
        ;(sleep 2);sleep two seconds
})