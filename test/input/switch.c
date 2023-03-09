int check_password(const char *passwd, int len) {
  switch (len) {
    case 5:
      if (passwd[0] == 'L') {
        if (passwd[1] == 'M') {
          if (passwd[2] == 'F') {
            if (passwd[3] == 'A') {
              if (passwd[4] == 'O') {
                return 1;
              }
            }
          }
        }
      }
      break;

    case 3:
      if (passwd[0] == 'L') {
        if (passwd[1] == 'O') {
          if (passwd[2] == 'L') {
            return 1;
          }
        }
      }
      break;

    default:
      break;
  }

  return 0;
}
