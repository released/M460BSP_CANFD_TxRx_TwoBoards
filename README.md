# M460BSP_CANFD_TxRx_TwoBoards
 M460BSP_CANFD_TxRx_TwoBoards

update @ 2023/12/12

1. init CAN interface , under 2 EVM : 

- DEVICE_A : M467 IOT , ID : 0x44444

- DEVICE_B : M463 EVB , ID : 0x33333

2. By using terminal , press digit 1 , to send CAN TX , another board will display receive CAN RX result 

below is enable define : ENABLE_CAN_FD

when under device A , send CAN TX to device B 

![image](https://github.com/released/M460BSP_CANFD_TxRx_TwoBoards/blob/main/board_A_to_B.jpg)

![image](https://github.com/released/M460BSP_CANFD_TxRx_TwoBoards/blob/main/board_A_to_B_II.jpg)


when under device B , send CAN TX to device A 

![image](https://github.com/released/M460BSP_CANFD_TxRx_TwoBoards/blob/main/board_B_to_A.jpg)

3. 

below is enable define : ENABLE_CAN_NORMAL

when under device A , send CAN TX to device B 

![image](https://github.com/released/M460BSP_CANFD_TxRx_TwoBoards/blob/main/board_A_to_B_CAN_NORMAL.jpg)


when under device B , send CAN TX to device A 

![image](https://github.com/released/M460BSP_CANFD_TxRx_TwoBoards/blob/main/board_B_to_A_CAN_NORMAL.jpg)

