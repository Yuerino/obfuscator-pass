int check_password(const char *passwd, int len) {
  if (len != 5) {
    return 0;
  }

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
  return 0;
}
