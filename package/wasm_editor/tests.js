const WAITING_TIME_MAX = 2000;

async function expect(page, fn, expected, name) {
    const where = new Error().stack.split('\n')[3].trim();
    await page.waitForFunction(`(${fn})() === ${expected}`, null, { timeout: WAITING_TIME_MAX }).catch(e => {
        throw new Error(name + ' failed: ' + where + '\n' + e.message);
    });
}

function EXPECT_TRUE(page, fn) {
    return expect(page, fn, true, 'EXPECT_TRUE');
}

function EXPECT_FALSE(page, fn) {
    return expect(page, fn, false, 'EXPECT_FALSE');
}

function EXPECT_VISIBLE(page, id) {
    return expect(page, `() => testing_visible('${id}')`, true, 'EXPECT_VISIBLE');
}

function EXPECT_HIDDEN(page, id) {
    return expect(page, `() => testing_visible('${id}')`, false, 'EXPECT_HIDDEN');
}

async function ACTION(page, id, value) {
    const where = new Error().stack.split('\n')[2].trim();
    const clickable = await page.evaluate(id => document.getElementById(id)?.onclick != null, id);
    const done = clickable
        ? page.click('#' + id, { timeout: WAITING_TIME_MAX }).then(() => true)
        : page.evaluate(([id, value]) => testing_action(id, value), [id, value]);
    if (await done.catch(e => { throw new Error('ACTION failed: ' + id + ' ' + where + '\n' + e.message); }) == false) {
        throw new Error('ACTION failed: ' + id + ' ' + where);
    }
}

module.exports = {
    test_node_menu_is_opened: async page => {
        await EXPECT_VISIBLE(page, 'button_node_new');
        await ACTION(page, 'button_node_new');
        await EXPECT_VISIBLE(page, 'add_node_menu');
        await ACTION(page, 'node_tree');
        await EXPECT_HIDDEN(page, 'add_node_menu');
    },
};
