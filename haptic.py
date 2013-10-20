import usb_comm

def main():

    # Initialize usb
    husb = usb_comm.usb_comm()

    while(1):
        # [current_val, emf_val, fb_val, enc_count_val] = husb.get_vals()
        print husb.get_vals()

if __name__ == '__main__':
    main()
