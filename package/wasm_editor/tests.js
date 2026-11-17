const WAITING_TIME_MAX = 100;

async function expect(page, fn, expected, name) {
    const where = new Error().stack.split('\n')[3].trim();
    await page.waitForFunction(`(${fn})() === ${expected}`, null, { timeout: WAITING_TIME_MAX }).catch(() => {
        throw new Error(name + ' failed: ' + where);
    });
}

function EXPECT_TRUE(page, fn) {
    return expect(page, fn, true, 'EXPECT_TRUE');
}

function EXPECT_FALSE(page, fn) {
    return expect(page, fn, false, 'EXPECT_FALSE');
}

module.exports = {
    test_node_menu_is_opened: async page => {
        await EXPECT_TRUE(page, () => testing_visible('button_node_new'));
        await EXPECT_TRUE(page, () => testing_action('button_node_new'));
        await EXPECT_TRUE(page, () => testing_visible('add_node_menu'));
    },
};
