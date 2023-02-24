int main() {
  int ret = 0;
  for (int i = 0; i < 1; i++) {
    for (int j = 0; j < 2; j++) {
      ret++;

      while (ret < 3) {
        ret++;
        while (ret < 4) {
          ret++;
        }
      }
    }

    while (ret < 5) {
      ret++;
      while (ret < 6) {
        ret++;
      }
    }

    do {
      ret++;
    } while (ret < 7);

  test1:
    if (ret < 8) {
      ret++;
      goto test1;
    }

  test2:
    ret++;
    if (ret < 9) {
      goto test2;
    }

    for (int l = 0; l < 10; l++) {
      if (l == 11) {
        continue;
      }
      ret++;
    }
  }

  for (int i = 0; i < 12; i++) {
    for (int j = 0; j < 13; j++) {
      ret++;

      while (ret < 14) {
        ret++;
      }
    }
  }

  while (ret < 15) {
    ret++;
    while (ret < 16) {
      ret++;
    }
  }

  int x[] = {1, 2};
  int z[] = {2, 3};
  for (int a = 0, b = 0; a < 17 && b < 18; a++, b++) {
    x[a] = z[b] + x[a];
  }

  return ret;
}
